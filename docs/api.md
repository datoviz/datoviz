# C API documentation

## App API

### `vkl_app()`

=== "C"
    ```c
    VklApp* vkl_app(VklBackend backend);
    ```

Create an application instance.

There is typically only one App object in a given application. This object holds a pointer to
the Vulkan instance and is responsible for discovering the available GPUs.

| Parameter | Type | Description |
| ---- | ---- | ---- |
| :octicons-arrow-right-16: `backend` | `VklBackend` | the backend |
| :octicons-arrow-left-16: `returns` | `VklApp*` | pointer to the created app |

### `vkl_app_destroy()`

=== "C"
    ```c
    int vkl_app_destroy(VklApp* app);
    ```

Destroy the application.

This function automatically destroys all objects created within the application.

| Parameter | Type | Description |
| ---- | ---- | ---- |
| :octicons-arrow-right-16: `app` | `VklApp*` | the application to destroy |
