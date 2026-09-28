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


def get_job():
    req_url = url+"?&cmd=get_job"
    if src_name is not None:
        req_url += "&src_name=" + src_name

    result = requests.get(req_url)
    if result.status_code != 200:
        result.raise_for_status()
    return result.text


def key_exists(key):
    for s in solvers:
        if s['key'] == key and not s['pause']:
            return True
    return False


def save_job(hex_key, content):
    req_url = url+f"?&cmd=save_job&key={hex_key}"
    if src_name is not None:
        req_url += "&src_name=" + src_name

    headers = {'Content-Type': 'application/x-www-form-urlencoded'}
    result = requests.post(req_url, data=content, headers=headers)
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
                    save_job_str = solver_str
                    save_job(s['key'], save_job_str)

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


src_name = os.getenv("src_name")

try:
    wait_cycle()
except Exception as e:
    print(e)

for s in solvers:
    if not s['pause']:
        print(f'kill key={s["key"]}')
        s['p'].kill()



