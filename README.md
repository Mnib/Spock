# Spock

A minimal library for Vulkan.
![screenshot](https://github.com/user-attachments/assets/d6ae0686-bd5c-4478-9aa9-7490d8a61c8e)

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
