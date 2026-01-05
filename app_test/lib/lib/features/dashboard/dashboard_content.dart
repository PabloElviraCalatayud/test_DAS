import 'dart:async';
import 'package:flutter/material.dart';
import '../../data/sensors/imu/imu_store.dart';
import '../../data/sensors/pulse/pulse_store.dart';
import 'widgets/imu_card.dart';
import 'widgets/pulse_card.dart';

class DashboardContent extends StatefulWidget {
  final ImuStore imuStore;
  final PulseStore pulseStore;

  const DashboardContent({
    super.key,
    required this.imuStore,
    required this.pulseStore,
  });

  @override
  State<DashboardContent> createState() => _DashboardContentState();
}

class _DashboardContentState extends State<DashboardContent> {
  StreamSubscription? _imuSub;
  StreamSubscription? _pulseSub;

  @override
  void initState() {
    super.initState();

    _imuSub = widget.imuStore.stream.listen((_) {
      if (mounted) setState(() {});
    });

    _pulseSub = widget.pulseStore.stream.listen((_) {
      if (mounted) setState(() {});
    });
  }

  @override
  void dispose() {
    _imuSub?.cancel();
    _pulseSub?.cancel();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    final imuState = widget.imuStore.state;
    final pulseState = widget.pulseStore.state;

    return ListView(
      padding: const EdgeInsets.all(16),
      children: [
        ImuCard(state: imuState),
        const SizedBox(height: 16),
        PulseCard(state: pulseState),
      ],
    );
  }
}
