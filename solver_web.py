import subprocess
import time
import os
import requests
import joblib

url = "http://93.127.143.124/solution/solve.php"
solver = './solver'

process_count = joblib.cpu_count()
solvers = []
span_count = 0
src_name = None


def get_jobs(cnt):
    req_url = url+"?cmd=get_job&cnt="+str(cnt)
    if src_name is not None:
        req_url += "&src_name=" + src_name

    result = requests.get(req_url, timeout=300)
    if result.status_code != 200:
        result.raise_for_status()

    content = result.text.split('\n')
    ret = []

    for c in content:
        c = c.strip()
        if c != '':
            ret.append(c)

    return ret


def key_exists(key):
    for s in solvers:
        if not s['pause'] and s['key'] == key:
            return True
    return False


def save_job(hex_key, content):
    req_url = url+f"?cmd=save_job&key={hex_key}"
    if src_name is not None:
        req_url += "&src_name=" + src_name

    result = requests.post(req_url, data=content, timeout=300)
    if not result.ok:
        result.raise_for_status()


def span_solver(hex_key, idx):
    global span_count
    span_count += 1

    print(f'span_solver() {idx=} {span_count=} {hex_key=}')
    log_file = open(f'solver{idx}.log', "ab")
    log_file.write(f'\n\nhex_key: {hex_key}\n'.encode("utf-8"))
    log_file.close()
    log_file = open(f'solver{idx}.log', "ab")
    ret = {'err': log_file, 'key': hex_key, 'pause': False}
    ret['p'] = subprocess.Popen([solver, hex_key], stdout=subprocess.PIPE, stderr=ret['err'])

    return ret


def wait_cycle():
    global solvers
    global process_count

    for i in range(0, process_count):
        solvers.append({'key': '', 'pause': True})

    while True:
        save_job_str = ''
        for i in range(0, len(solvers)):
            s = solvers[i]
            if s['pause']:
                continue

            try:
                s['p'].wait(0)
            except subprocess.TimeoutExpired as e:
                continue

            if s['p'].returncode != 0:
                raise Exception(f'solver failed {i=} key={s["key"]}')
            else:
                solver_str = s['p'].communicate()[0].decode("utf-8")
                save_job_str += solver_str + "\n"

            s['err'].close()
            s['pause'] = True

        if save_job_str != '':
            try:
                save_job(s['key'], save_job_str)
            except Exception as ex:
                print(ex)

        paused_count = 0
        for i in range(0, len(solvers)):
            s = solvers[i]
            if s['pause']:
                paused_count = paused_count + 1

        if paused_count > 0:
            try:
                hex_keys = get_jobs(paused_count)
            except Exception as ex:
                print(ex)
                break

            j = 0
            for i in range(0, len(solvers)):
                if j >= len(hex_keys):
                    break

                s = solvers[i]
                if not s['pause']:
                    continue

                hex_key = hex_keys[j]
                j = j + 1

                if key_exists(hex_key):
                    print(f'key={hex_key} already processing')
                else:
                    s = span_solver(hex_key, i)

                solvers[i] = s

        time.sleep(0.1)


def kill_all():
    global solvers

    for s in solvers:
        if not s['pause']:
            print(f'kill key={s["key"]}')
            s['p'].kill()


src_name = os.getenv("src_name")

try:
    wait_cycle()
except Exception as e:
    print(e)

kill_all()



