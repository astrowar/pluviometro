import socket
import time
import requests
import json
import datetime
import pickle
from subprocess import call


def internet(host="8.8.8.8", port=53, timeout=3) -> bool:
    try:
        socket.setdefaulttimeout(timeout)
        socket.socket(socket.AF_INET, socket.SOCK_STREAM).connect((host, port))
        return True

    except Exception as ex:
        print(ex.message)

    return False


def wait_for_internet(timeout=50) -> bool:
    acc_time = 0
    while not internet():
        time.sleep(5)
        acc_time += 5
        if acc_time > timeout:
            return False
    return True


def get_stored() -> list:
    try:
        data = pickle.load(open("counter.pickle", "rb"))
        return data
    except (OSError, IOError) as e:
        data = []
        pickle.dump(data, open("counter.pickle", "wb"))
        return []


def get_arduino_serial_counter():
    return 1


def get_arduino() -> list:
    counter = get_arduino_serial_counter()

    now = datetime.datetime.now()
    dd = str(now.day)
    mm = str(now.month)
    if len(dd) < 2: dd = '0' + dd
    if len(mm) < 2: mm = '0' + mm
    return [(str(now.year) + mm + dd, str(now.hour), counter)]


def reset_arduino_counters() -> None:
    # GPIO.send(HIGH,pin6)
    pass


def add_counter(ct, counters=None) -> list:
    if counters is None:
        counters = []
    has_add = False
    for i in range(len(counters)):
        c = counters[i]
        if c[0] == ct[0]:
            if c[1] == ct[1]:
                counters[i] = (counters[i][0], counters[i][1], counters[i][2] + ct[2])
                has_add = True
    if not has_add:
        counters.append(ct)
    return counters


def get_counters() -> list:
    counters = []
    cts1 = get_stored()
    for ct in cts1:
        counters = add_counter(ct, counters)
    cts2 = get_arduino()
    for ct in cts2:
        counters = add_counter(ct, counters)
    return counters


def send_azure_table_item(counter) -> bool:
    # specify url
    data = counter[0]
    hour = counter[1]
    mm = counter[2]
    url = 'http://pluviometrosaojoseriopardo.azurewebsites.net/api/addHora/' + str(data) + '/' + str(hour) + '/' + str(
        mm)
    print(url)
    token = "my token"
    data = {
        "agentName": "myAgentName",
        "agentId": "20",
        "description": "Changed Description",
        "platform": "Windows"
    }
    headers = {'Authorization': 'Bearer ' + token, "Content-Type": "application/json"}
    # Call REST API
    response = requests.put(url, data=data, headers=headers)
    # Print Response
    print(response.text)
    dct = json.loads(response.text)
    if 'date' in dct:
        print('sucess')
        return True
    print('fail')
    return False
    # reset_arduino_counters()


def send_azure_table(counters) -> list:
    keep = []
    for c in counters:
        if not send_azure_table_item(c):
            keep.append(c)
    return keep


def store_counter(counters) -> None:
    print("store:", counters)
    pickle.dump(counters, open("counter.pickle", "wb"))


def processing():
    counters = get_counters()
    print(counters)
    has_internet = wait_for_internet()
    if has_internet:
        keep = send_azure_table(counters)
        store_counter(keep)
    else:
        store_counter(counters)
    reset_arduino_counters()


processing()
call("sudo nohup shutdown -h now", shell=True)
