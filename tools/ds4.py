#!/usr/bin/env python3
import usb.core
import usb.util
import struct
import time

VENDOR_ID  = 0x054c
PRODUCT_ID = 0x09cc

dev = usb.core.find(idVendor=0x054c, idProduct=0x09cc)
if dev is None:
    raise IOError("DS4 not found")

cfg = dev.get_active_configuration()
# 1) 모든 인터페이스에서 커널 드라이버 분리 + claim
for intf in cfg:
    if dev.is_kernel_driver_active(intf.bInterfaceNumber):
        dev.detach_kernel_driver(intf.bInterfaceNumber)
    usb.util.claim_interface(dev, intf.bInterfaceNumber)

# 2) 읽을 인터럽트 엔드포인트 찾기
sensor_ep = None
for intf in cfg:
    for ep in intf:
        if usb.util.endpoint_direction(ep.bEndpointAddress) == usb.util.ENDPOINT_IN \
           and usb.util.endpoint_type(ep.bmAttributes) == usb.util.ENDPOINT_TYPE_INTR:
            sensor_ep = ep
            iface_no  = intf.bInterfaceNumber
            break
    if sensor_ep:
        break
if not sensor_ep:
    raise IOError("Interrupt endpoint not found")

print(f"Using interface {iface_no}, endpoint 0x{sensor_ep.bEndpointAddress:02x}")
print("Reading reports (Ctrl+C to stop)…")

# 4) 읽기 루프
start = time.monotonic()
try:
    while True:
        # bEndpointAddress, wMaxPacketSize
        data = dev.read(sensor_ep.bEndpointAddress, sensor_ep.wMaxPacketSize, timeout=5000)
        t = time.monotonic() - start

        # 스틱 축
        lx, ly = data[1], data[2]
        rx, ry = data[3], data[4]
        # 트리거
        l2, r2 = data[8], data[9]

        # 자이로(20-25), 가속도(28-33)
        # Gyro (bytes 20–25)

        gx, gy, gz = struct.unpack_from("<hhh", data, 13)
        ax, ay, az = struct.unpack_from('<hhh', data, 19)
        # 배터리 (byte30, USB 모드)
        raw_batt = data[12]


        print(f"{t:6.3f}s | LS({lx:3d},{ly:3d}) RS({rx:3d},{ry:3d}) "
              f"L2={l2:3d} R2={r2:3d} | "
              f"G:({gx:6d},{gy:6d},{gz:6d}) A:({ax:6d},{ay:6d},{az:6d}) "
              f"Batt:{raw_batt:3d}%")

        time.sleep(0.002)

except KeyboardInterrupt:
    print("\nStopped by user.")
