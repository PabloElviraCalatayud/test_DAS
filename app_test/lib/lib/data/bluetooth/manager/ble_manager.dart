import 'dart:async';
import 'dart:typed_data';

import 'package:flutter_blue_plus/flutter_blue_plus.dart';

import '../../../core/constants/ble_constants.dart';
import '../../../features/packet/logic/packet_decoder.dart';
import '../../../features/ota/ota_sender.dart';
import '../../../core/utils/ble_foreground.dart';

enum BleConnectionState {
  idle,
  scanning,
  connecting,
  connected,
}

StreamSubscription<BluetoothConnectionState>? _connSub;
StreamSubscription<List<int>>? _notifySub;

bool _autoReconnect = true;
int _reconnectAttempt = 0;

class BleManager {
  BleManager._internal();

  static final BleManager instance = BleManager._internal();

  BluetoothDevice? connectedDevice;
  BluetoothCharacteristic? _notifyChar;
  BluetoothCharacteristic? _writeChar;

  BluetoothCharacteristic? get writeChar => _writeChar;

  BluetoothCharacteristic? get notifyChar => _notifyChar;

  BleConnectionState _state = BleConnectionState.idle;
  final StreamController<BleConnectionState> _stateCtrl =
  StreamController.broadcast();

  Stream<BleConnectionState> get stateStream => _stateCtrl.stream;

  BleConnectionState get state => _state;

  bool _otaActive = false;

  void _setState(BleConnectionState s) {
    _state = s;
    _stateCtrl.add(s);
  }

  void scan() {
    FlutterBluePlus.stopScan();
    _setState(BleConnectionState.scanning);

    FlutterBluePlus.startScan(
      timeout: const Duration(seconds: 5),
      androidUsesFineLocation: true,
    );
  }

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
    //try { await device.removeBond(); } catch (_) {}

    await device.connect(
      autoConnect: false,
      timeout: const Duration(seconds: 15),
    );

    await device.connectionState.firstWhere(
          (s) => s == BluetoothConnectionState.connected,
    );

    connectedDevice = device;

    _connSub?.cancel();
    _connSub = device.connectionState.listen((s) {
      if (s == BluetoothConnectionState.disconnected) {
        _setState(BleConnectionState.idle);
        if (_autoReconnect && connectedDevice != null) {
          _reconnect();
        }
      }
    });

    await device.requestMtu(247);
    await _discoverCharacteristics();

    await BleForeground.start();

    _setState(BleConnectionState.connected);
  }

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

    await _notifyChar!.setNotifyValue(true);
    await _notifySub?.cancel();
    _notifySub = _notifyChar!.lastValueStream.listen((data) {
      print('[BLE] notify len=${data.length} first=${data.isNotEmpty ? data[0] : -1}');
      final bytes = Uint8List.fromList(data);

      if (_otaActive) {
        OtaSender.handleNotify(bytes);
      } else {
        PacketDecoder.instance.decode(bytes);
      }
    });
  }

  Future<void> startOtaMode() async {
    _otaActive = true;
  }

  Future<void> endOtaMode() async {
    _otaActive = false;
  }

  Future<void> disconnect() async {
    _autoReconnect = false;
    await _notifySub?.cancel();
    _notifySub = null;
    await _connSub?.cancel();
    _connSub = null;

    if (connectedDevice == null) {
      _setState(BleConnectionState.idle);
      await BleForeground.stop();
      return;
    }

    try {
      await _notifyChar?.setNotifyValue(false);
    } catch (_) {}

    try {
      await connectedDevice!.disconnect();
    } catch (_) {}

    connectedDevice = null;
    _notifyChar = null;
    _writeChar = null;
    _otaActive = false;

    await BleForeground.stop();
    _setState(BleConnectionState.idle);
    _autoReconnect = true;
    _reconnectAttempt = 0;
  }


  Future<void> _reconnect() async {
    final dev = connectedDevice;
    if (dev == null) return;

    _reconnectAttempt++;
    final delaySeconds = [1, 2, 5, 10, 20, 30]
        .elementAt((_reconnectAttempt - 1).clamp(0, 5));
    await Future.delayed(Duration(seconds: delaySeconds));

    try {
      _setState(BleConnectionState.connecting);
      await dev.connect(
          autoConnect: false, timeout: const Duration(seconds: 15));
      await dev.connectionState.firstWhere(
            (s) => s == BluetoothConnectionState.connected,
      );

      _reconnectAttempt = 0;
      await dev.requestMtu(247);
      await _discoverCharacteristics();
      _setState(BleConnectionState.connected);
    } catch (_) {
      // sigue intentando mientras el usuario no desconecte manualmente
      if (_autoReconnect && connectedDevice != null) {
        _reconnect();
      }
    }
  }
}