# Spock

A minimal library for Vulkan.

## Getting started

Compile with

```bash
cmake \
    -DCMAKE_BUILD_TYPE=Debug \
    -DENABLE_ASAN=OFF \
    -DBUILD_EXAMPLES=ON \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
    .
make -j$(nproc)
```

## Running

You can run the example in `SpockApp`
