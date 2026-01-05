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
              onPressed: (_file != null && !_sending)
                  ? _sendFirmware
                  : null,
            ),
          ],
        ),
      ),
    );
  }

  // ------------------------------------------------
  // FILE PICKER
  // ------------------------------------------------
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

  // ------------------------------------------------
  // OTA SEND
  // ------------------------------------------------
  Future<void> _sendFirmware() async {
    final writeChar = BleManager.instance.writeChar;

    if (writeChar == null || _file == null) {
      ScaffoldMessenger.of(context).showSnackBar(
        const SnackBar(content: Text('BLE no preparado')),
      );
      return;
    }

    setState(() {
      _sending = true;
      _progress = 0.0;
    });

    try {
      final sender = OtaSender(writeChar);

      await sender.sendFirmware(
        _file!,
            (p) {
          setState(() {
            _progress = p;
          });
        },
      );

      if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          const SnackBar(content: Text('OTA completada, reiniciando dispositivo')),
        );
      }

    } catch (e) {
      if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(content: Text('Error OTA: $e')),
        );
      }
    } finally {
      if (mounted) {
        setState(() {
          _sending = false;
        });
      }
    }
  }
}
