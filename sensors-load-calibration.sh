#!/system/bin/sh

# AL3010 Ambient Light Sensor calibration data
if [ -f /per/lightsensor/AL3010_Config.ini ]; then
    cat /per/lightsensor/AL3010_Config.ini > /sys/devices/platform/tegra-i2c.2/i2c-2/2-001c/calibration
fi

# AMI304 Compass calibration data
if [ -f /per/sensors/AMI304_Config.ini ]; then
    cat /per/sensors/AMI304_Config.ini > /sys/devices/platform/tegra-i2c.2/i2c-2/2-000e/iio:device1/compass_cali_data
fi
