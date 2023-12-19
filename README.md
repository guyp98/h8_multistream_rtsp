# h8_multistream_rtsp

## Overview

This repository includes scripts and examples for testing a Real-Time Streaming Protocol (RTSP) send script and receiving the stream using the Hailo-8 multistream solution. The provided instructions guide you through setting up and executing the RTSP send script and the multistream Hailo-8 receiver script. For users preferring C++, an alternative method is available with detailed build and run instructions.

## Prerequisites

Before using this repository, ensure the following prerequisites are met:

- [Tappas](https://github.com/hailo-ai/tappas) installed. Follow the installation instructions in the Tappas documentation.

  To run Tappas from prebuilt binaries in a Docker container, refer to the [Docker installation guide](https://github.com/hailo-ai/tappas/blob/master/docs/installation/docker-install.rst).

  To build Tappas locally, follow the instructions in the [Tappas documentation](https://github.com/hailo-ai/tappas/blob/master/docs/installation/docker-install.rst).

## Testing Procedure

### first run the rtsp send script:
```
./send_rtsp.sh
```

### second to thecive the streames you have two options:

1. Multistream Hailo-8 Receiver Script:
Run the multistream Hailo-8 receiver script to receive and process the RTSP stream.
```
multistream_hailo.sh
```

2. C++ Implementation:
Navigate to the C++ implementation directory.
```
cd cpp_impl
```
Build the C++ implementation.
```
./build.sh
```
Run the multistream Hailo-8 executable.
```
./build/multistream_hailo
```
#### C++ Implementation Details
The C++ example features a callback function that is invoked for every frame passing through the processing pipeline. This callback mechanism ensures that custom actions can be performed on each frame. Two distinct methods are available for capturing frames, namely "probs" and "appink." Users can specify their preferred mode by adjusting the "read_from" variable within the code.

Feel free to explore and customize the provided scripts and examples according to your specific requirements
