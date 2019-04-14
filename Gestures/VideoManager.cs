using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Windows.Media.Capture;
using Windows.Media.Capture.Frames;
using Windows.Graphics.Imaging;
using Windows.UI.Xaml.Media.Imaging;
using Windows.UI.Xaml.Controls;
using Windows.UI.Core;

namespace Gestures
{
    class VideoManager
    {
        public SoftwareBitmap frame;
        public SoftwareBitmap processedFrame;

        public Image previewImage;
        public Image processedImage;

        private MediaCapture capture = null;
        private MediaFrameReader reader = null;
        private OpenCVBridge.OpenCVHelper helper;

        public VideoManager(Image nPreviewImage, Image nProcessedimage)
        {
            this.previewImage = nPreviewImage;
            this.processedImage = nProcessedimage;
            previewImage.Source = new SoftwareBitmapSource();
            processedImage.Source = new SoftwareBitmapSource();

            helper = new OpenCVBridge.OpenCVHelper();
        }

        public async Task InitMedia()
        {
            var groups = await MediaFrameSourceGroup.FindAllAsync();

            capture = new MediaCapture();
            var settings = new MediaCaptureInitializationSettings()
            {
                SourceGroup = groups[0],
                SharingMode = MediaCaptureSharingMode.SharedReadOnly,
                StreamingCaptureMode = StreamingCaptureMode.Video,
                MemoryPreference = MediaCaptureMemoryPreference.Cpu
            };
            await capture.InitializeAsync(settings);

            MediaFrameSource frameSource;
            try
            {
                frameSource = capture.FrameSources.Values.First(x => x.Info.SourceKind == MediaFrameSourceKind.Color);
            } catch
            {
                frameSource = capture.FrameSources.Values.First();
            }
            reader = await capture.CreateFrameReaderAsync(frameSource);
            reader.FrameArrived += HandleFrames;
            await reader.StartAsync();
        }

        public async Task StopMedia()
        {
            await capture.StopPreviewAsync();
            await reader.StopAsync();
        }

        private void HandleFrames(MediaFrameReader reader, MediaFrameArrivedEventArgs args)
        {
            var newFrame = reader.TryAcquireLatestFrame();
            if (newFrame == null) return;
            frame = SoftwareBitmap.Convert(newFrame.VideoMediaFrame.SoftwareBitmap, BitmapPixelFormat.Bgra8, BitmapAlphaMode.Premultiplied);
            SoftwareBitmap outputBitmap = new SoftwareBitmap(BitmapPixelFormat.Bgra8, frame.PixelWidth, frame.PixelHeight, BitmapAlphaMode.Premultiplied);
            SoftwareBitmap previewFlipped = new SoftwareBitmap(BitmapPixelFormat.Bgra8, frame.PixelWidth, frame.PixelHeight, BitmapAlphaMode.Premultiplied);

            ProcessFrame(frame, outputBitmap);

            helper.Flip(frame, previewFlipped);

            RenderFrame(previewImage, previewFlipped);
            RenderFrame(processedImage, outputBitmap);
        }

        private void ProcessFrame(SoftwareBitmap inputBitmap, SoftwareBitmap outputBitmap)
        {
            helper.Process(inputBitmap, outputBitmap);
        }

        public void RenderFrame(Image element, SoftwareBitmap sb)
        {
            if (sb.BitmapPixelFormat != BitmapPixelFormat.Bgra8)
                sb = SoftwareBitmap.Convert(sb, BitmapPixelFormat.Bgra8);

            try
            {
                var task = previewImage.Dispatcher.RunAsync(CoreDispatcherPriority.Normal,
                    async () => {
                        var imageSource = (SoftwareBitmapSource)element.Source;
                        await imageSource.SetBitmapAsync(sb);
                    });
            }
            catch
            {
                return;
            }
        }
    }
}
