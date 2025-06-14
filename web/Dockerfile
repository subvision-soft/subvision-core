FROM emscripten/emsdk:2.0.10

ENV OPENCV_VERSION=4.11.0

RUN wget https://github.com/opencv/opencv/archive/refs/tags/${OPENCV_VERSION}.tar.gz && \
    tar xf ${OPENCV_VERSION}.tar.gz && \
    cd opencv-${OPENCV_VERSION}/ && \
    emcmake python3 ./platforms/js/build_js.py build_js --cmake_option="-DOPENCV_GENERATE_PKGCONFIG=ON -DBUILD_opencv_imgcodecs=OFF" && \
    cd build_js && \
    make install && \
    cd /src && \
    rm -r opencv-${OPENCV_VERSION} ${OPENCV_VERSION}.tar.gz && \
    apt update && \
    apt install --no-install-recommends -y pkg-config && \
    rm -rf /var/lib/apt/lists/*