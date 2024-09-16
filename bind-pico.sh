#!/bin/bash

VID="2e8a"
PID="000a"
INTERFACE_NUM="1.3"  # Replace with your gs_usb interface number
BAD_IF_NUM="1.2"
# Find the sysfs path for the device using VID and PID
SYSFS_DEVICE_PATH=$(find -L /sys/bus/usb/devices/ -maxdepth 2 -name idVendor -exec grep -l "$VID" {} \; | xargs -I{} dirname {} | xargs -I{} grep -l "$PID" {}/idProduct | xargs -I{} dirname {})

if [ -z "$SYSFS_DEVICE_PATH" ]; then
    echo "Device with VID=${VID} and PID=${PID} not found"
    exit 1
fi

# Extract the base device name, e.g., 1-1.2
DEVICE_PATH=$(basename "$SYSFS_DEVICE_PATH")

# Construct the full device path with the interface number, e.g., 1-1.2:1.0
FULL_DEVICE_PATH="${DEVICE_PATH}:${INTERFACE_NUM}"
BFDP="${DEVICE_PATH}:${INTERFACE_NUM}"

echo "Unbinding from current driver (if any)..."
# Unbind the device from its current driver, if any
if [ -e "/sys/bus/usb/devices/${FULL_DEVICE_PATH}/driver/unbind" ]; then
    echo -n "${FULL_DEVICE_PATH}" > /sys/bus/usb/devices/${FULL_DEVICE_PATH}/driver/unbind
fi

echo "${VID} ${PID}"
echo "Binding ${FULL_DEVICE_PATH} to gs_usb driver"

# Bind to the gs_usb driver
echo -n "${VID} ${PID}" > /sys/bus/usb/drivers/gs_usb/new_id
# echo -n "${FULL_DEVICE_PATH}" > /sys/bus/usb/drivers/gs_usb/bind
echo -n "${BFDP}" > /sys/bus/usb/drivers/gs_usb/unbind

echo "Device bound to gs_usb driver"