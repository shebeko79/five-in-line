// src\ThreadProcessor.cpp : implementation file
//

#include "stdafx.h"
#include "ThreadProcessor.h"
#include <boost/function.hpp>
#include <boost/bind.hpp>
//#include "extern/exception_catch.h"
#include <stdexcept>
#include ".\threadprocessor.h"

// CThreadProcessor

IMPLEMENT_DYNAMIC(CThreadProcessor, CWnd)
CThreadProcessor::CThreadProcessor()
{
	window_created=false;
    state = st_stopped;
}

CThreadProcessor::~CThreadProcessor()
{
	stop();
}


BEGIN_MESSAGE_MAP(CThreadProcessor, CWnd)
	ON_MESSAGE(WM_COMPLETE,OnCompleteHandler)
END_MESSAGE_MAP()

void CThreadProcessor::start()
{
	stop();

	create_window();

	state=st_wait_task;

	try
	{
		thrd=boost::shared_ptr<boost::thread>(new boost::thread(
			boost::bind(&CThreadProcessor::execute,this)));
	}
	catch(...)
	{
		state=st_stopped;
		throw;
	}
}

void CThreadProcessor::stop()
{
	if(!thrd)return;
	
	{
		lkt lk(mtx);
		state=st_stopping;
		cond.notify_all();
	}
	thrd->join();
	thrd.reset();

	state=st_stopped;
}

void CThreadProcessor::execute()
{
	try
	{
		while(true)
		{
            if(!wait_for_new_job())
                return;
            
            OnExecute();

			{
				lkt lk(mtx);
				if(state==st_stopping)return;
				state=st_wait_task;
				PostMessage(WM_COMPLETE,0,0);
                cond.notify_all();
			}
		}
	}
	catch(std::exception& e)
	{
		if(state==st_stopping)return;
		error_t err;
		err.message=std::string("std::exception: ")+e.what();
		add_message(err);
		PostMessage(WM_COMPLETE,0,0);
	}
	catch(...)
	{
		if(state==st_stopping)return;
		error_t err;
		err.message="unknown exception";
		add_message(err);
		PostMessage(WM_COMPLETE,0,0);
	}
}

bool CThreadProcessor::wait_for_new_job()
{
    lkt lk(mtx);
    while(state!=st_new_task)
    {
        if(state==st_stopping)
            return false;

        cond.wait(lk);
    }

    state=st_execute_task;
    return true;
}


void CThreadProcessor::start_job()
{
    {
        lkt lk(mtx);
        if(state!=st_wait_task)throw std::runtime_error("CThreadProcessor::start_job(): invalid state");
        state=st_new_task;
    }
	cond.notify_all();
}

bool CThreadProcessor::is_job_in_progress() const
{
    lkt lk(mtx);
    return state==st_new_task || state==st_execute_task;
}

void CThreadProcessor::cancel_job()
{
	lkt lk(mtx);
	switch(state)
	{
	case st_wait_task:
		return;
	case st_new_task:
		state=st_wait_task;
		return;
	case st_execute_task:
		break;
    default:
        throw std::runtime_error("CThreadProcessor::cancel_job(): invalid state");
	}

    while(state==st_execute_task)
    {
        cond.wait(lk);
    }
}

void CThreadProcessor::add_message(const error_t& mess)
{
	lkt lk(mtx);
	messages.push_back(mess);
}


void CThreadProcessor::create_window()
{
	if(window_created)return;
	if(!Create(0,"",WS_CHILD,CRect(0,0,1,1),AfxGetMainWnd( ),0,0))
		throw std::runtime_error("CThreadProcessor::create_window() failed");
	window_created=true;
}

LRESULT CThreadProcessor::OnCompleteHandler(WPARAM,LPARAM)
{
	try
	{
		if(state==st_stopping||!thrd)return 0;
		errors_t vals_messages;
		
		{
			lkt lk(mtx);

			std::swap(messages,vals_messages);
		}
		if(vals_messages.empty())OnComplete();
		else OnErrors(vals_messages);
	}
	catch(...)
	{
	}

	return 0;
}
