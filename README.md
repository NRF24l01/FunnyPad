# FunnyPad

Soundpad but for linux, for pavucontrol

```shell
pactl load-module module-null-sink sink_name=SoundpadSink sink_properties=device.description=SoundpadSink
pactl load-module module-remap-source master=SoundpadSink.monitor source_name=VirtualMic source_properties=device.description=VirtualMic
```