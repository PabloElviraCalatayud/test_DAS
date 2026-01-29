import 'package:flutter_foreground_task/flutter_foreground_task.dart';

class BleForeground {
  static bool _started = false;

  static Future<void> start() async {
    if (_started) return;

    FlutterForegroundTask.init(
      androidNotificationOptions: AndroidNotificationOptions(
        channelId: 'ble_stream',
        channelName: 'BLE Streaming',
        channelDescription: 'Mantiene la conexión BLE activa en segundo plano',
        channelImportance: NotificationChannelImportance.LOW,
        priority: NotificationPriority.LOW,
      ),
      iosNotificationOptions: const IOSNotificationOptions(),
      foregroundTaskOptions: ForegroundTaskOptions(
        // En v8.6+ esto ES obligatorio. Sustituye al antiguo "interval".
        // 5000 ms = cada 5s (no hace nada, pero mantiene el “loop” del servicio).
        eventAction: ForegroundTaskEventAction.repeat(5000),
        allowWakeLock: true,
        allowWifiLock: false,
        autoRunOnBoot: false,
        autoRunOnMyPackageReplaced: false,
      ),
    );

    await FlutterForegroundTask.startService(
      notificationTitle: 'Dispositivo conectado',
      notificationText: 'Recibiendo datos por BLE…',
      callback: _startCallback,
    );

    _started = true;
  }

  static Future<void> stop() async {
    if (!_started) return;
    await FlutterForegroundTask.stopService();
    _started = false;
  }
}

@pragma('vm:entry-point')
void _startCallback() {
  FlutterForegroundTask.setTaskHandler(_NoopTaskHandler());
}

class _NoopTaskHandler extends TaskHandler {
  @override
  Future<void> onStart(DateTime timestamp, TaskStarter starter) async {}

  @override
  Future<void> onRepeatEvent(DateTime timestamp) async {
    // No hacemos nada: el objetivo es mantener el foreground service vivo.
  }

  @override
  Future<void> onDestroy(DateTime timestamp) async {}

  @override
  void onNotificationPressed() {}

  @override
  void onButtonPressed(String id) {}
}
