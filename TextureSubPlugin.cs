using System;
using System.Runtime.InteropServices;

namespace TextureSubPlugin {

    [StructLayout(LayoutKind.Sequential)]
    public struct TextureSubImage2DParams {
        public IntPtr texture_handle;
        public Int32 xoffset;
        public Int32 yoffset;
        public Int32 width;
        public Int32 height;
        public IntPtr data_ptr;
        public Int32 level;
        public Int32 format;
    };

    [StructLayout(LayoutKind.Sequential)]
    public struct TextureSubImage3DParams {
        public IntPtr texture_handle;
        public Int32 xoffset;
        public Int32 yoffset;
        public Int32 zoffset;
        public Int32 width;
        public Int32 height;
        public Int32 depth;
        public IntPtr data_ptr;
        public Int32 level;
        public Int32 format;
    };

    [StructLayout(LayoutKind.Sequential)]
    public struct CreateTexture3DParams {
        public UInt32 texture_id;
        public UInt32 width;
        public UInt32 height;
        public UInt32 depth;
        public Int32 format;
    };

    [StructLayout(LayoutKind.Sequential)]
    public struct DestroyTexture3DParams {
        public UInt32 texture_id;
    };

    public enum Event : Int32 {
        TextureSubImage2D = 0,
        TextureSubImage3D = 1,
        CreateTexture3D = 2,
        DestroyTexture3D = 3
    };

    public enum Format : Int32 {
        UR8 = 0,
        UR16 = 1
    }

    public static class API {
        [DllImport("TextureSubPlugin")]
        public static extern IntPtr GetRenderEventFunc();

        [DllImport("TextureSubPlugin")]
        public static extern IntPtr RetrieveCreatedTexture3D(UInt32 texture_id);
    };
}
