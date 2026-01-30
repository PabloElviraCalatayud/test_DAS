import 'package:flutter/material.dart';
import '../../../core/services/telemetry_starter.dart';
import '../../../data/sensors/imu/imu_sample.dart';

class DebugTbScreen extends StatefulWidget {
  const DebugTbScreen({super.key});

  @override
  State<DebugTbScreen> createState() => _DebugTbScreenState();
}

class _DebugTbScreenState extends State<DebugTbScreen> {
  final TelemetryStarter telemetryStarter = TelemetryStarter();

  bool _isSendingPulse = false;
  bool _isSendingImu = false;

  int? _lastPulse;
  ImuSample? _lastImu;
  String _pulseStatus = '';
  String _imuStatus = '';

  Future<void> _sendPulse() async {
    setState(() => _isSendingPulse = true);
    try {
      final pulse = await telemetryStarter.sendPulseTelemetry();
      setState(() {
        _lastPulse = pulse;
        _pulseStatus = ' Pulso $pulse enviado';
      });
    } catch (e) {
      setState(() => _pulseStatus = ' Error al enviar pulso');
    } finally {
      setState(() => _isSendingPulse = false);
    }
  }

  Future<void> _sendImu() async {
    setState(() => _isSendingImu = true);
    try {
      final sample = await telemetryStarter.sendImuTelemetry();
      setState(() {
        _lastImu = sample;
        _imuStatus = ' IMU enviada';
      });
    } catch (e) {
      setState(() => _imuStatus = ' Error al enviar IMU');
    } finally {
      setState(() => _isSendingImu = false);
    }
  }

//muestra ultimo envio de imu
  Widget _buildImuDisplay() {
    if (_lastImu == null) return const SizedBox.shrink();

    final imu = _lastImu!;
    return Column(
      children: [
        const SizedBox(height: 8),
        const Text(
          'IMU (m/s² / °/s):',
          style: TextStyle(fontWeight: FontWeight.bold),
        ),
        Text(
            'Acc: (${imu.axMs2.toStringAsFixed(2)}, ${imu.ayMs2.toStringAsFixed(2)}, ${imu.azMs2.toStringAsFixed(2)})'),
        Text(
            'Gyro: (${imu.gxDps.toStringAsFixed(2)}, ${imu.gyDps.toStringAsFixed(2)}, ${imu.gzDps.toStringAsFixed(2)})'),
      ],
    );
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(title: const Text("Debug ThingsBoard")),
      body: Center(
        child: SingleChildScrollView(
          child: Column(
            mainAxisAlignment: MainAxisAlignment.center,
            children: [
              const Icon(Icons.cloud_upload, size: 80, color: Colors.blueGrey),
              const SizedBox(height: 20),

              _isSendingPulse
                  ? const CircularProgressIndicator()
                  : ElevatedButton.icon(
                onPressed: _sendPulse,
                icon: const Icon(Icons.favorite),
                label: const Text('Enviar Pulso'),
              ),
              const SizedBox(height: 8),
              Text(_pulseStatus, style: const TextStyle(fontSize: 16)),

              const Divider(height: 30, thickness: 2),
              _isSendingImu
                  ? const CircularProgressIndicator()
                  : ElevatedButton.icon(
                onPressed: _sendImu,
                icon: const Icon(Icons.sensors),
                label: const Text('Enviar IMU'),
              ),
              Text(_imuStatus, style: const TextStyle(fontSize: 16)),
              // Mostrar último sample IMU
              _buildImuDisplay(),
            ],
          ),
        ),
      ),
    );
  }
}