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
    return ['2018:05:28:8:47:25', '2018:05:28:9:40:11', '2018:05:28:11:40:53', '2018:05:28:8:40:12']


def get_arduino() -> list:
    logs = get_arduino_serial_counter()
    ret = []
    for s in logs:
        s.split(":")
        _year = s[0]
        _month = s[1]
        _day = s[2]
        _h = s[3]
        if len(_day) < 2: _day = '0' + _day
        if len(_month) < 2: _month = '0' + _month
        if len(_h) < 2: _h = '0' + _h
        ret.append( (_year + _month + _day, _h, 1) )
    return ret


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


if __name__ == "__main__":
    processing()
    call("sudo nohup shutdown -h now", shell=True)
