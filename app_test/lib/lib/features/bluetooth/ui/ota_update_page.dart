import 'dart:io';

import 'package:flutter/material.dart';
import 'package:file_picker/file_picker.dart';

class OtaUpdatePage extends StatefulWidget {
  const OtaUpdatePage({super.key});

  @override
  State<OtaUpdatePage> createState() => _OtaUpdatePageState();
}

class _OtaUpdatePageState extends State<OtaUpdatePage> {
  File? _file;

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
              onPressed: _pickFile,
              child: const Text('Seleccionar archivo .bin'),
            ),

            const SizedBox(height: 16),

            if (_file != null)
              Text(
                _file!.path.split('/').last,
                textAlign: TextAlign.center,
              ),

            const SizedBox(height: 24),

            ElevatedButton.icon(
              icon: const Icon(Icons.upload),
              label: const Text('Enviar firmware'),
              onPressed: _file != null
                  ? _sendFirmware
                  : null,
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

  void _sendFirmware() {
    // AQUÍ irá la llamada al OtaSender
    // Ejemplo futuro:
    // OtaSender(BleManager.instance.writeChar!).sendFirmware(_file!, ...);

    ScaffoldMessenger.of(context).showSnackBar(
      const SnackBar(
        content: Text('Envío OTA aún no conectado'),
      ),
    );
  }
}
