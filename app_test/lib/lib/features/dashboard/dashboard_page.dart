import 'package:flutter/material.dart';

import '../../data/sensors/imu/imu_store.dart';
import '../../data/sensors/pulse/pulse_store.dart';
import 'dashboard_content.dart';

class DashboardPage extends StatelessWidget {
  const DashboardPage({super.key});

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('Dashboard'),
      ),
      body: DashboardContent(
        imuStore: ImuStore.instance,
        pulseStore: PulseStore.instance,
      ),
    );
  }
}
