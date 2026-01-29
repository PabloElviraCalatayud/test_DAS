import 'package:flutter/material.dart';

import '../../data/health/health_store.dart';
import '../../data/sensors/pulse/pulse_store.dart';
import 'health_content.dart';

class HealthPage extends StatelessWidget {
  const HealthPage({super.key});

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('Salud'),
      ),
      body: HealthContent(
        healthStore: HealthStore.instance,
        pulseStore: PulseStore.instance,
      ),
    );
  }
}