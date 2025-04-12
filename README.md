# Spock

A minimal library for Vulkan.
![screen](https://github.com/user-attachments/assets/8ea13617-e1ae-419c-8d3c-c0847d6c155b)

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
