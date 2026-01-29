import 'dart:io';

import 'package:flutter/material.dart';
import 'package:file_picker/file_picker.dart';

import '../../../data/bluetooth/manager/ble_manager.dart';
import '../../ota/ota_sender.dart';

class OtaUpdatePage extends StatefulWidget {
  const OtaUpdatePage({super.key});

  @override
  State<OtaUpdatePage> createState() => _OtaUpdatePageState();
}

class _OtaUpdatePageState extends State<OtaUpdatePage> {
  File? _file;
  double _progress = 0.0;
  bool _sending = false;

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(title: const Text('Actualización OTA')),
      body: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.stretch,
          children: [
            ElevatedButton(
              onPressed: _sending ? null : _pickFile,
              child: const Text('Seleccionar archivo .bin'),
            ),
            const SizedBox(height: 16),
            if (_file != null)
              Text(
                _file!.path.split('/').last,
                textAlign: TextAlign.center,
              ),
            const SizedBox(height: 24),
            LinearProgressIndicator(
              value: _sending ? _progress : null,
              minHeight: 8,
            ),
            const SizedBox(height: 24),
            ElevatedButton.icon(
              icon: const Icon(Icons.upload),
              label: Text(_sending ? 'Enviando…' : 'Enviar firmware'),
              onPressed:
              (_file != null && !_sending) ? _sendFirmware : null,
            ),
          ],
        ),
      ),
    );
  }

  Future<void> _pickFile() async {
    final result = await FilePicker.platform.pickFiles(
      type: FileType.custom,
      allowedExtensions: ['bin'],
    );

    if (result != null && result.files.single.path != null) {
      setState(() {
        _file = File(result.files.single.path!);
      });
    }
  }

  Future<void> _sendFirmware() async {
    final ble = BleManager.instance;
    final writeChar = ble.writeChar;
    final notifyChar = ble.notifyChar;

    if (writeChar == null || notifyChar == null || _file == null) {
      return;
    }

    setState(() {
      _sending = true;
      _progress = 0.0;
    });

    try {
      await ble.startOtaMode();

      final sender = OtaSender(writeChar, notifyChar);

      await sender.sendFirmware(
        _file!,
            (p) {
          setState(() {
            _progress = p;
          });
        },
      );
    } finally {
      await ble.endOtaMode();
      if (mounted) {
        setState(() {
          _sending = false;
        });
      }
    }
  }
}
