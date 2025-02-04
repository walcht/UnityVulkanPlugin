# About
TextureSubPlugin is a Unity low-level native plugin for uploading data to chunks
(i.e., subregions or bricks) of a 2D/3D Texture. The plugin can also be used to
create 3D/2D textures which is quite handy for circumventing Unity's
Texture2D/3D 2GBs size limitation.

This repository is adapted from [Unity native rendering plugin repository](https://github.com/Unity-Technologies/NativeRenderingPlugin/tree/master).

## Building the Plugin

1.  build the plugin using CMake. Assuming you are in this repo's root:

    ```bash
    mkdir build && cd build
    cmake ..
    ```
    
    if you are on Windows, then the easiest way to configure the build is using
    cmake-gui and opening the generated build files with Visual Studio.
    
    if you are targeting Android then provide cmake with the NDK cmake toolchain
    file and additional NDK arguments
    (see [ndk-cmake-guide](https://developer.android.com/ndk/guides/cmake)):

    ```bash
    mkdir build && cd build
    cmake .. -DCMAKE_TOOLCHAIN_FILE=$NDK/build/cmake/android.toolchain.cmake \
        -DANDROID_ABI=x86_64 -DANDROID_PLATFORM=android-29 \
        -DANDROID_STL=c++_shared
    ```

1. navigate to the built TextureSubPlugin.so (libTextureSubPlugin.so on Linux)
   in your build directory and copy it to your
   `<unity-project-dir>/Assets/Plugins/` folder

1. add the attached `TextureSubPlugin.cs` to your Unity project for
   plugin-related enums, structs, and API declarations.


## Usage

### Texture Creation

For textures larger than 2GBs and when using OpenGL or Vulkan, using Unity's
Texture3D/2D constructor outputs the following error:

```Texture3D (WIDTHxHEIGHTxDEPTH) is too large, currently up to 2GB is allowed```

Direct3D11/12 impose a 2GB limit per resource but OpenGL and Vulkan don't.
To bypass this, create the texture using the provided ```CreateTexture3D```
(see TextureSubPluginAPI.h):

```csharp
// create a command buffer in which graphics commands will be submitted
CommandBuffer cmd_buffer = new();

// create and keep track of a unique texture ID - this is used later on to
// retrieve the create texture
UInt32 texture_id = 0;

// allocate the plugin call's arguments struct
CreateTexture3DParams args = new() {
    texture_id = texture_id,
    width = ...,
    height = ...,
    depth = ...,
    format = ...,
};
IntPtr p_args = Marshal.AllocHGlobal(Marshal.SizeOf<CreateTexture3DParams>());
Marshal.StructureToPtr(args, p_args, false);
cmd_buffer.IssuePluginEventAndData(TextureSubPlugin.API.GetRenderEventFunc(),
    (int)TextureSubPlugin.Event.CreateTexture3D, p_args);

// execute the command buffer immediately
Graphics.ExecuteCommandBuffer(cmd_buffer);

// we can be sure that the texture has been updated
yield return new WaitForEndOfFrame();

Marshal.FreeHGlobal(p_args);

// retrieve the create texture using the assigned texture ID
IntPtr native_tex_ptr = API.RetrieveCreatedTexture3D(texture_id);
if (native_tex_ptr == IntPtr.Zero) {
    throw new NullReferenceException("native texture pointer is nullptr. " +
        "Make sure that your platform supports native code plugins");
}

// finally create the actual Unity Texture 3D object by supplying a pointer
// to the externaly create texture
Texture3D tex = Texture3D.CreateExternalTexture(..., ..., ..., ...,
    mipChain: ..., nativeTex: native_tex_ptr);
```

If you have used ```CreateTexture3D``` to create a native texture then you
should also call ```DestroyTexture3D``` to destroy it. Not doing so will result
in a GPU memory leak (e.g., your GPU memory usage will increase each time
you toggle the play mode in Unity editor):

```csharp
// create a command buffer in which graphics commands will be submitted
CommandBuffer cmd_buffer = new();

DestroyTexture3DParams args = new () {
    texture_handle = tex.GetNativeTexturePtr()
};

IntPtr p_args = Marshal.AllocHGlobal(Marshal.SizeOf<CreateTexture3DParams>());
Marshal.StructureToPtr(args, p_args, false);
cmd_buffer.IssuePluginEventAndData(TextureSubPlugin.API.GetRenderEventFunc(),
    (int)TextureSubPlugin.Event.DestroyTexture3D, p_args);

// execute the command buffer immediately
Graphics.ExecuteCommandBuffer(cmd_buffer);

// only free allocated resources in next frame
yield return new WaitForEndOfFrame();

Marshal.FreeHGlobal(p_args);
```

Again, if the graphics API is Direct3D11/12, there is (probably) no good reason
to use ```CreateTexture3D```.

### Texture Update

The following example illustrates how to update a subregion of a 3D texture
using this plugin:

```csharp
// wait until the rendering thread is done working
yield return new WaitForEndOfFrame();

// create a command buffer in which graphics commands will be submitted
CommandBuffer cmd_buffer = new();

// data is the sub-region data array
byte[] data = new byte[DATA_LENGTH];

// data is a managed object and we want to pass it to unmanaged code. The GC
// could in theory move the data's object data around in memory. To avoid this,
// a GCHandle has to be created for data that instructs GC to keep the data
// pinned to the same memory address location. When we are done we the data, we
// have to inform the GC that it is free to manage the object as it wants.
GCHandle handle = GCHandle.Alloc(data, GCHandleType.Pinned);

// allocate the plugin call's arguments struct
TextureSubImage3DParams args = new() {
    texture_handle = tex.GetNativeTexturePtr(),
    xoffset = ...,
    yoffset = ...,
    zoffset = ...,
    width = ...,
    height = ...,
    depth = ...,
    data_ptr = handle.AddrOfPinnedObject(),
    level = ...,
    format = ... 
};
IntPtr p_args = Marshal.AllocHGlobal(Marshal.SizeOf<TextureSubImage3DParams>());
Marshal.StructureToPtr(args, p_args, false);
cmd_buffer.IssuePluginEventAndData(TextureSubPlugin.API.GetRenderEventFunc(),
    (int)TextureSubPlugin.Event.TextureSubImage3D, p_args);

// execute the command buffer immediately
Graphics.ExecuteCommandBuffer(cmd_buffer);

// we can be sure that the texture has been updated
yield return new WaitForEndOfFrame();

Marshal.FreeHGlobal(p_args);

// hey GC, you are again free to manage the data object
handle.Free();
```

## License

MIT License. Read `license.txt` file.
