import subprocess
import time
import os

db_path = 'e:/f5'
solver = 'solver/cmake/Release/solver.exe'
db = 'db/cmake/Release/db.exe'
base_state = '(0,0:X);(-4,-4:O);(-1,0:X)'

process_count = os.cpu_count()
solvers = []
span_count = 0


def get_job():
    result = subprocess.run([db, db_path, 'get_ant_job', base_state], capture_output=True, text=True)
    if result.returncode != 0:
        raise Exception(f'get_job(): ret={result.stdout}')
    return result.stdout


def save_job(content):
    with open("save_job.log", "w", encoding="utf-8") as file:
        file.write(content)

    result = subprocess.run([db, db_path, 'save_job', "save_job.log"], capture_output=True, text=True)
    if result.returncode != 0:
        raise Exception(f'save_job(): ret={result.stdout}')


def span_solver(hex_key, idx):
    global span_count
    span_count += 1

    print(f'span_solver() {idx=} {span_count=} {hex_key=}')
    ret = {'err': open(f'solver{idx}.log', "wb"), 'key': hex_key}
    ret['p'] = subprocess.Popen([solver, hex_key], stdout=subprocess.PIPE, stderr=ret['err'])

    return ret


def solver_output2save_job(res_str):
    parts = res_str.split('&')
    k = ''
    n = ''
    w = ''
    f = ''

    for p in parts:
        eq=p.split('=')
        if len(eq) !=2:
            continue

        if eq[0] == 'k':
            k = eq[1]
        elif eq[0] == 'n':
            n = eq[1]
        elif eq[0] == 'w':
            w = eq[1]
        elif eq[0] == 'f':
            f = eq[1]

    ret = f'{k};{n};{w};{f}'
    return ret


def initial_span():
    for i in range(0, process_count):
        hex_key = get_job()
        solvers.append(span_solver(hex_key, i))


def wait_cycle():
    while True:
        for i in range(0, process_count):
            s = solvers[i]
            try:
                s['p'].wait(0)
            except subprocess.TimeoutExpired as e:
                continue

            s['err'].close()
            if s['p'].returncode != 0:
                raise Exception(f'solver failed {i=} key={s["key"]}')
            else:
                solver_str = s['p'].communicate()[0].decode("utf-8")
                save_job_str = solver_output2save_job(solver_str)
                save_job(save_job_str)

            hex_key = get_job()
            solvers[i] = span_solver(hex_key, i)

        time.sleep(1)


initial_span()
wait_cycle()


