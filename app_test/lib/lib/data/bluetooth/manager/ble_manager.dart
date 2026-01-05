import 'dart:async';
import 'dart:typed_data';

import 'package:flutter/cupertino.dart';
import 'package:flutter_blue_plus/flutter_blue_plus.dart';

import '../../../core/constants/ble_constants.dart';
import '../../../features/packet/logic/packet_decoder.dart';

enum BleConnectionState {
  idle,
  scanning,
  connecting,
  connected,
}

class BleManager {
  BleManager._internal();
  static final BleManager instance = BleManager._internal();

  BluetoothDevice? connectedDevice;
  BluetoothCharacteristic? _notifyChar;
  BluetoothCharacteristic? _writeChar;

  BluetoothCharacteristic? get writeChar => _writeChar;

  BleConnectionState _state = BleConnectionState.idle;

  final StreamController<BleConnectionState> _stateCtrl =
  StreamController.broadcast();

  Stream<BleConnectionState> get stateStream => _stateCtrl.stream;
  BleConnectionState get state => _state;

  void _setState(BleConnectionState s) {
    _state = s;
    _stateCtrl.add(s);
  }

  // ------------------------------------------------
  // SCAN
  // ------------------------------------------------
  void scan() {
    FlutterBluePlus.stopScan();

    _setState(BleConnectionState.scanning);

    FlutterBluePlus.startScan(
      timeout: const Duration(seconds: 5),
      androidUsesFineLocation: true,
    );
  }

  // ------------------------------------------------
  // CONNECT
  // ------------------------------------------------
  Future<void> connect(BluetoothDevice device) async {
    if (_state == BleConnectionState.connecting ||
        _state == BleConnectionState.connected) {
      return;
    }

    _setState(BleConnectionState.connecting);

    FlutterBluePlus.stopScan();

    try {
      await device.disconnect();
    } catch (_) {}

    try {
      await device.removeBond();
    } catch (_) {}

    await device.connect(
      autoConnect: false,
      timeout: const Duration(seconds: 15),
    );

    await device.connectionState.firstWhere(
          (s) => s == BluetoothConnectionState.connected,
    );

    connectedDevice = device;

    await device.requestMtu(247);
    await _discoverCharacteristics();

    _setState(BleConnectionState.connected);
  }

  Future<void> disconnect() async {
    try {
      await connectedDevice?.disconnect();
    } catch (_) {}

    connectedDevice = null;
    _notifyChar = null;
    _writeChar = null;

    _setState(BleConnectionState.idle);
  }

  // ------------------------------------------------
  // DISCOVER
  // ------------------------------------------------
  Future<void> _discoverCharacteristics() async {
    final services = await connectedDevice!.discoverServices();

    for (final s in services) {
      if (s.uuid.toString().toLowerCase() !=
          BleConstants.serviceUuid.toLowerCase()) {
        continue;
      }

      for (final c in s.characteristics) {
        final uuid = c.uuid.toString().toLowerCase();

        if (uuid == BleConstants.txCharUuid.toLowerCase()) {
          _notifyChar = c;
        }

        if (uuid == BleConstants.rxCharUuid.toLowerCase()) {
          _writeChar = c;
        }
      }
    }

    if (_notifyChar == null || _writeChar == null) {
      throw Exception('TX/RX characteristics not found');
    }

    await _enableNotifications();
  }

  // ------------------------------------------------
  // NOTIFICATIONS
  // ------------------------------------------------
  Future<void> _enableNotifications() async {
    if (_notifyChar == null) {
      throw Exception('Notify characteristic not set');
    }

    await _notifyChar!.setNotifyValue(true);

    _notifyChar!.value.listen((data) {
      debugPrint('RX chunk: ${data.length} bytes');
      PacketDecoder.instance.decode(Uint8List.fromList(data));
    });
  }
}
