# Realtime QR code decoding with ESP32

## What is this?

I recently purchased ESP-EYE modules to check the capabilities of ESP32 for real time vision algorithms. ESP32 is suprisingly good in terms of transfering the JPEG encoded video stream to a web browser without doing any unpacking or processing on it. But again, this is due to the efficiency of the JPEG encoding of the camera and the network stack of ESP32.

QR code detection runs with almost 1FPS detection speed. I used [https://github.com/dlbeer/quirc](https://github.com/dlbeer/quirc) library for detection. It is not very optimised for embedded systems. Even though I gave rather large stack size for the detection task, system resets due to _Stack overflow_ now and then. Also I changed _double_ precision calculations to _float_ inside the quirc library and tried to put almost all the library methods inside RAM with IRAM_ATTR to gain marginal speed increase. Also, getting the image in RGB space in JPEG format from the camera, unpacking it to raw databuffer, then converting that raw databuffer into raw grayscale image buffer is far from an optimal image pipeline. On the other hand, JPEG frame capture works the best in terms of frame rate in this system. Go figure.

My long term goal was to check whether I can run Apriltag decoder on this system like OpenMV ([https://www.youtube.com/watch?v=keb0B11zj5g](https://www.youtube.com/watch?v=keb0B11zj5g)) but after seeing the QR decoding performance, I decided not to pursue Apriltag decoding. ESP32 _may_ be useful for color based object tracking projects with a decent frame rate, but other than that I probably wont be spending any more time.

## How?

Setup the wifi ssid pass information in the main.c file. After compiling and loading program, run the "udplisten.py" script to listen to the port number 12345 for learning the IP address of the ESP32 module. Then go to http://{ip_address}/live_stream address to run the demo.

## Test Video

[https://vimeo.com/406752608](https://vimeo.com/406752608)

## Details

* ESP-IDF version is: v4.0
* Hardware platform is: [https://www.espressif.com/en/products/hardware/esp-eye/overview](https://www.espressif.com/en/products/hardware/esp-eye/overview)
* Similar project: [https://github.com/donny681/ESP32_CAMERA_QR](https://github.com/donny681/ESP32_CAMERA_QR)
