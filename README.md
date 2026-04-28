# Skeletal Animation Minimal Bundle

This folder contains only the files needed to **run** the `8.guest_2020_skeletal_animation` program:

- `bin/8.guest/2020/skeletal_animation/8.guest_2020_skeletal_animation`
- shaders next to the executable (`anim_model.vs/.fs`, `hp_bar.vs/.fs`)
- required assets under `resources/` (FarmerPack + vampire)

## Run

From this folder:

```bash
export LOGL_ROOT_PATH="$PWD"
./bin/8.guest/2020/skeletal_animation/8.guest_2020_skeletal_animation
```

If you run it from a different working directory, keep `LOGL_ROOT_PATH` pointing to this folder.

## Source

Minimal source is included under:

- `src/8.guest/2020/skeletal_animation/`
- `includes/` (subset copied from the main repo)

To build from source (macOS example):

```bash
cmake -S . -B build
cmake --build build -j
export LOGL_ROOT_PATH="$PWD"
./build/8.guest_2020_skeletal_animation
```
# 3D_animation
