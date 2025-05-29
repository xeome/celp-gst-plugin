# CELP Microphone Test Commands

## Quick Tests

### 1. Live Microphone → CELP → Speaker
```bash
export GST_PLUGIN_PATH=$PWD/build:$GST_PLUGIN_PATH
gst-launch-1.0 pulsesrc ! audioconvert ! audioresample ! audio/x-raw,format=S16LE,rate=8000,channels=1 ! celpenc ! celpdec ! audioconvert ! pulsesink
```
## Audio Quality Notes

- **Input**: Any format/rate → automatically converted to S16LE, 8000Hz, mono
- **CELP Encoding**: 2.4 kbps bitrate (144 bits per 240-sample frame)
- **Output Quality**: Similar to phone call quality, optimized for voice
- **Latency**: Minimal for real-time applications

## Troubleshooting

If you get "No such element" errors:
```bash
# Check if plugin is loaded
gst-inspect-1.0 celpenc
gst-inspect-1.0 celpdec

# Check plugin path
echo $GST_PLUGIN_PATH
ls -la build/libgstcelp.so
```

If you get audio device errors:
```bash
# List available audio devices
gst-device-monitor-1.0 Audio/Source
gst-device-monitor-1.0 Audio/Sink

# Use specific device (replace device.name with actual device)
gst-launch-1.0 pulsesrc device="alsa_input.pci-0000_00_1f.3.analog-stereo" ! ...