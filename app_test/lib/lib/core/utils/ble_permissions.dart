import 'package:permission_handler/permission_handler.dart';

class BlePermissions {
  static Future<bool> ensure() async {
    final statuses = await [
      Permission.bluetoothScan,
      Permission.bluetoothConnect,
      Permission.location,
    ].request();

    return statuses.values.every((s) => s.isGranted);
  }
}
