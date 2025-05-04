#!/usr/bin/env python3
import threading
import time

import pyudev
from evdev import InputDevice, ecodes

def read_loop(devnode: str):
    """
    지정된 /dev/input/eventX 디바이스를 열어
    ABS 이벤트 중 코드 2(L2), 5(L3) 값을 출력
    """
    try:
        dev = InputDevice(devnode)
        print(f"▶ Started reading events from {devnode}")
        for ev in dev.read_loop():
            if ev.type == ecodes.EV_ABS:
                if ev.code == 2:       # L2 트리거
                    print(f"L2 trigger value: {ev.value}")
                elif ev.code == 5:     # L3 트리거
                    print(f"L3 trigger value: {ev.value}")
    except Exception as e:
        print(f"⚠️  Thread({devnode}) 종료: {e}")

def is_ds4(device):
    """
    udev.Device에서 DualShock4인지 판별
    """
    return (
        device.get('ID_VENDOR_ID') == '054c' and
        device.get('ID_INPUT_JOYSTICK') == '1' and
        device.device_node and
        device.device_node.startswith('/dev/input/event')
    )

def start_read_thread(devnode, handlers):
    """
    중복 없이 read_loop 스레드 시작
    """
    if devnode in handlers:
        return
    t = threading.Thread(target=read_loop, args=(devnode,), daemon=True)
    handlers[devnode] = t
    t.start()

def main():
    context = pyudev.Context()
    handlers = {}

    # 1) 초기: 이미 연결된 모든 input 디바이스 순회
    for device in context.list_devices(subsystem='input'):
        if is_ds4(device):
            devnode = device.device_node
            print(f"▶ Found existing DualShock: {devnode}")
            start_read_thread(devnode, handlers)

    # 2) udev 모니터 준비
    monitor = pyudev.Monitor.from_netlink(context)
    monitor.filter_by('input')
    monitor.start()

    print("▶ Monitoring for new DualShock connections… (Ctrl+C to exit)")

    # 3) 새로 부착되는 디바이스 감지 루프
    for device in monitor:
        action = device.action
        if action != 'add':
            continue
        if is_ds4(device):
            devnode = device.device_node
            print(f"✅ DualShock connected: {devnode}")
            start_read_thread(devnode, handlers)
        time.sleep(0.1)

if __name__ == "__main__":
    main()
