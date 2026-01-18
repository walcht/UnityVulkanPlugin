# About
*UnityVulkanPlugin* is a Unity low-level native plugin for accessing low-level Vulkan
API using C++ within Unity. For instance, this can be used to upload data to chunks
(i.e., subregions or bricks) of a 2D/3D Texture which is not possible with the exposed
high-level Unity C# API. The plugin can also be used to create 3D/2D textures which is
quite handy for circumventing Unity's Texture2D/3D 2GBs size limitation.

This repository is adapted from [Unity native rendering plugin repository](https://github.com/Unity-Technologies/NativeRenderingPlugin/tree/master).
Rather than having multiple build files, this project makes use of a single
build generator system using CMake.

Due to the high effort needed to maintain multiple graphics APIs, this plugin now
only supports Vulkan. If you want to see how you can support other APIs, see
the branch *old-multiple-apis*.

## Building the Plugin

1.  build the plugin using CMake. Assuming you are in the same folder as this
    README:

    ```bash
    mkdir build
    cd build
    # or on Windows powershell: $env:UNITY_EDITOR_DATA_PATH="path-to-your-installed-unity-editor-data-folder"
    export UNITY_EDITOR_DATA_PATH=<path-to-your-installed-unity-editor-data-folder>
    cmake .. --preset <preset>
    cmake --build .
    cmake --install . --prefix <path-to-your-unity-project-plugins-folder>
    ```

    **UNITY_EDITOR_DATA_PATH** is the path to your Unity Editor data folder. You
    can get it by going to your Unity Hub > Installs > pick which installed editor you
    want to use > Show File in File Browser > navigate to the Data folder >
    copy absolute path (e.g., ```C:\Program Files\Unity\Hub\Editor\6000.0.40f1\Editor\Data```).
    (Note: The **same** Unity Editor that is used for building your project should be set
    here!)

    *preset* is the target you are building this plugin for. Check 
    ```CMakePresets.txt``` for the available presets and configuration.
    *preset* can be:

    - **windows** - in case **SUPPORT_OPENGL_CORE** is set, **GLAD_PATH** has to
    be filled with the path to [glad](https://glad.dav1d.de/)

    - **linux** - in case **SUPPORT_VULKAN** is set, Vulkan SDK has to be
    installed

    - **android** - Make sure that your Unity editor has the module **Android SDK & NDK Tools** installed.
    Additionally, **ANDROID_ABI**, **ANDROID_PLATFORM**, and **ANDROID_STL** have
    to be populated (go to CMakePresets.txt and populate them for your target)
    see [NDK cmake guide](https://developer.android.com/ndk/guides/cmake>).

    - **magicleap2** - same as android preset but with populated NDK arguments.
    This preset uses the Ninja generator so that cross-compilation on a Windows
    platform is working.

2. both the library files (TextureSubPlugin.so and libTextureSubPlugin.so) should
   now be available in the provided installation directory (have to be in
   Assets/Plugins for the plugin to be found and loaded)

3. in your Unity project, click on the installed .so/.dll native plugin file and
   fill the platform settings

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

// create the actual Unity Texture 3D object by supplying a pointer
// to the externally create texture
Texture3D tex = Texture3D.CreateExternalTexture(..., ..., ..., ...,
    mipChain: ..., nativeTex: native_tex_ptr);

// finally, make sure to overwrite native_tex_ptr
// this has to be overwritten for Vulkan to work because Unity expects a
// VkImage* for the nativeTex parameter not a VkImage. GetNativeTexturePtr does
// not actually return a VkImage* as it claims but rather a VkImage
// => this is probably a Unity bug.
// (see https://docs.unity3d.com/6000.0/Documentation/ScriptReference/Texture3D.CreateExternalTexture.html)
native_tex_ptr = m_brick_cache.GetNativeTexturePtr();
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

## Q&A

### Why do I get DllNotFoundException and how to solve it?

If you get a DllNotFoundException when trying your Unity build on your target,
the reasons might be:

- you compiled the plugin using a different compiler/toolchain than what you
compiled your Unity project, that uses the .so/.dll plugin, with. **Make sure
that you use the same compiler/toolchain. This is simply done by setting
UNITY_EDITOR_DATA_PATH to that of the same editor you are using to build your Unity
project.**

- refrain from renaming the build .so/.dll file and make sure that it is put
somewhere Unity can find it (usually Assets/Plugins).

- make sure that in Unity Editor, the installed .so/.dll has the correct
platform settings filled (CPU, OS, Include Platforms, etc.).

## License

MIT License. Read `license.txt` file.
