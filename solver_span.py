import subprocess
import time
import os

db_path = 'e:/f5'
solver = 'solver/cmake/Release/solver.exe'
db = 'db/cmake/Release/db.exe'
base_state = '(0,0:X);(-4,-4:O);(-1,0:X)'

process_count = os.cpu_count()
#process_count = 4
solvers = []
span_count = 0


def get_job():
    result = subprocess.run([db, db_path, 'get_ant_job', base_state], capture_output=True, text=True)
    if result.returncode != 0:
        raise Exception(f'get_job(): ret={result.stdout}')
    return result.stdout


def key_exists(key):
    for s in solvers:
        if s['key'] == key and not s['pause']:
            return True
    return False


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
    ret = {'err': open(f'solver{idx}.log', "wb"), 'key': hex_key, 'pause': False}
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


def wait_cycle():
    global solvers
    global process_count

    while True:
        for i in range(0, len(solvers)):
            s = solvers[i]
            if not s['pause']:
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

            s['pause'] = True

            hex_key = get_job()
            if key_exists(hex_key):
                print(f'key={hex_key} already processing')
            else:
                s = span_solver(hex_key, i)

            solvers[i] = s

        if len(solvers) < process_count:
            hex_key = get_job()
            if key_exists(hex_key):
                print(f'key={hex_key} already processing')
            else:
                solvers.append(span_solver(hex_key, len(solvers)))

        time.sleep(1)


try:
    wait_cycle()
except Exception as e:
    print(e)

for s in solvers:
    if not s['pause']:
        print(f'kill key={s["key"]}')
        s['p'].kill()



