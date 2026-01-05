import 'package:flutter/material.dart';
import 'package:flutter_blue_plus/flutter_blue_plus.dart';

import '../../../data/bluetooth/manager/ble_manager.dart';
import 'ota_update_page.dart';

class BleScanPage extends StatelessWidget {
  const BleScanPage({super.key});

  @override
  Widget build(BuildContext context) {
    final ble = BleManager.instance;

    return Scaffold(
      appBar: AppBar(title: const Text('Bluetooth')),
      body: StreamBuilder<BleConnectionState>(
        stream: ble.stateStream,
        initialData: ble.state,
        builder: (context, snap) {
          final state = snap.data!;

          if (state == BleConnectionState.connected) {
            return _connectedView(context, ble);
          }

          return _scanView(context, ble, state);
        },
      ),
    );
  }

  Widget _scanView(
      BuildContext context,
      BleManager ble,
      BleConnectionState state,
      ) {
    return Column(
      children: [
        Padding(
          padding: const EdgeInsets.all(16),
          child: Row(
            mainAxisAlignment: MainAxisAlignment.spaceEvenly,
            children: [
              ElevatedButton(
                onPressed: () {
                  ble.scan();
                },
                child: const Text('Iniciar escaneo'),
              ),
              ElevatedButton(
                onPressed: () {
                  FlutterBluePlus.stopScan();
                },
                child: const Text('Parar escaneo'),
              ),
            ],
          ),
        ),
        Expanded(
          child: StreamBuilder<List<ScanResult>>(
            stream: FlutterBluePlus.scanResults,
            builder: (context, snapshot) {
              final results = snapshot.data ?? [];

              if (results.isEmpty) {
                return const Center(
                  child: Text('No se han encontrado dispositivos'),
                );
              }

              return ListView(
                children: results.map((r) {
                  final advName = r.advertisementData.advName;
                  final name = advName.isNotEmpty
                      ? advName
                      : r.device.remoteId.str;

                  return ListTile(
                    leading: const Icon(Icons.bluetooth),
                    title: Text(name),
                    subtitle: Text(r.device.remoteId.str),
                    trailing: ElevatedButton(
                      onPressed: () async {
                        await FlutterBluePlus.stopScan();
                        await ble.connect(r.device);
                      },
                      child: const Text('Conectar'),
                    ),
                  );
                }).toList(),
              );
            },
          ),
        ),
      ],
    );
  }

  Widget _connectedView(BuildContext context, BleManager ble) {
    final device = ble.connectedDevice;

    return Padding(
      padding: const EdgeInsets.all(16),
      child: Column(
        children: [
          Card(
            child: ListTile(
              leading: const Icon(Icons.bluetooth_connected),
              title: Text(
                device?.platformName.isNotEmpty == true
                    ? device!.platformName
                    : device?.remoteId.str ?? 'Dispositivo',
              ),
              subtitle: Text(device?.remoteId.str ?? ''),
            ),
          ),
          const SizedBox(height: 24),
          ElevatedButton.icon(
            icon: const Icon(Icons.system_update),
            label: const Text('Actualizar firmware'),
            onPressed: () {
              Navigator.of(context).push(
                MaterialPageRoute(
                  builder: (_) => const OtaUpdatePage(),
                ),
              );
            },
          ),
          const SizedBox(height: 12),
          OutlinedButton(
            onPressed: () async {
              await ble.disconnect();
            },
            child: const Text('Desconectar'),
          ),
        ],
      ),
    );
  }
}
