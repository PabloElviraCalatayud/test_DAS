import 'package:flutter/material.dart';

import '../core/utils/onboarding_storage.dart';
import '../core/services/telemetry_starter.dart';
import 'app_shell.dart';
import '../features/onboarding/onboarding_page.dart';

class App extends StatelessWidget {
  const App({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      debugShowCheckedModeBanner: false,
      title: 'Monitor de Apnea',

      // Tu tema moderno
      theme: ThemeData(
        useMaterial3: true,
        brightness: Brightness.light,
        colorScheme: ColorScheme.fromSeed(
          seedColor: const Color(0xFF2B5C9E),
          secondary: const Color(0xFF00BFA5),
          surface: const Color(0xFFF8F9FC),
        ),
        scaffoldBackgroundColor: const Color(0xFFF8F9FC),
        cardTheme: CardThemeData(
          elevation: 0,
          shape: RoundedRectangleBorder(
            borderRadius: BorderRadius.circular(20),
          ),
          color: Colors.white,
          margin: EdgeInsets.zero,
        ),
        appBarTheme: const AppBarTheme(
          backgroundColor: Color(0xFFF8F9FC),
          elevation: 0,
          centerTitle: true,
          scrolledUnderElevation: 0,
          titleTextStyle: TextStyle(
            color: Color(0xFF1A1C1E),
            fontSize: 20,
            fontWeight: FontWeight.w600,
          ),
        ),
      ),

      home: const _Bootstrap(),
    );
  }
}

class _Bootstrap extends StatefulWidget {
  const _Bootstrap();

  @override
  State<_Bootstrap> createState() => _BootstrapState();
}

class _BootstrapState extends State<_Bootstrap> {
  late final TelemetryStarter _telemetryStarter;

  @override
  void initState() {
    super.initState();

    // Importante: esto NO debe ir en build()
    _telemetryStarter = TelemetryStarter();

    // Si quieres, puedes envolverlo en microtask para no bloquear el arranque
    Future.microtask(() {
      _telemetryStarter.sendPulseTelemetry();
      _telemetryStarter.sendImuTelemetry();
    });
  }

  @override
  Widget build(BuildContext context) {
    return FutureBuilder<bool>(
      future: OnboardingStorage.hasSeen(),
      builder: (context, snapshot) {
        if (!snapshot.hasData) return const SizedBox.shrink();
        if (snapshot.data == true) return const AppShell();
        return const OnboardingPage();
      },
    );
  }
}
