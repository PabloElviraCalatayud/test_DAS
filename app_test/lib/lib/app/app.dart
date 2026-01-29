import 'package:flutter/material.dart';

import '../core/utils/onboarding_storage.dart';
import 'app_shell.dart';
import '../features/onboarding/onboarding_page.dart';
import '../core/services/telemetry_starter.dart';
class App extends StatelessWidget {
  const App({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      debugShowCheckedModeBanner: false,
      home: const _Bootstrap(),
    );
  }
}

class _Bootstrap extends StatelessWidget {
  const _Bootstrap();

  @override
  Widget build(BuildContext context) {
    final telemetryStarter = TelemetryStarter();
    telemetryStarter.sendPulseTelemetry();
    telemetryStarter.sendImuTelemetry();
    return FutureBuilder<bool>(
      future: OnboardingStorage.hasSeen(),
      builder: (context, snapshot) {
        if (!snapshot.hasData) {
          return const SizedBox.shrink();
        }

        if (snapshot.data == true) {
          return const AppShell();
        }

        return const OnboardingPage();
      },
    );
  }
}
