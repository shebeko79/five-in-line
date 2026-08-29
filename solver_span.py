import subprocess
import time

db_path = 'data/'
solver = 'solver/cmake/Release/solver.exe'
db = 'db/cmake/Release/db.exe'
base_state = '(0,0:X);(-4,-4:O);(-1,0:X)'

process_count = 4
solvers = []


def get_job():
    result = subprocess.run([db, db_path, 'get_ant_job', base_state], capture_output=True, text=True)
    if result.returncode != 0:
        print("get_job(): ", result.stdout)
        return ''
    return result.stdout


def save_job(content):
    with open("save_job.txt", "w", encoding="utf-8") as file:
        file.write(content)

    result = subprocess.run([db, db_path, 'save_job', "save_job.txt"], capture_output=True, text=True)
    if result.returncode != 0:
        print("save_job(): ", result.stdout)


def span_solver(hex_key):
    return subprocess.Popen([solver, hex_key], stdout=subprocess.PIPE)


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
        solvers.append(span_solver(hex_key))


def wait_cycle():
    while True:
        for i in range(0, process_count):
            p = solvers[i]
            try:
                p.wait(0)
            except subprocess.TimeoutExpired as e:
                continue

            print(f'job complete {i=}\n')
            if p.returncode != 0:
                print('solver failed\n')
            else:
                solver_str = p.communicate()[0].decode("utf-8")
                save_job_str = solver_output2save_job(solver_str)
                save_job(save_job_str)

            hex_key = get_job()
            print(f'span new {hex_key=}\n')
            solvers[i] = span_solver(hex_key)

        time.sleep(1)


initial_span()
wait_cycle()


