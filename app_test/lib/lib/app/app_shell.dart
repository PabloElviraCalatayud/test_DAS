import 'package:flutter/material.dart';

import '../features/Health/health_page.dart';
import '../features/bluetooth/ui/ble_scan_page.dart';
import '../features/debug/debug_screen.dart';
import '../features/debug/widgets/debug_tb.dart';

class AppShell extends StatefulWidget {
  const AppShell({super.key});

  @override
  State<AppShell> createState() => _AppShellState();
}

class _AppShellState extends State<AppShell> {
  int _index = 0;

  final List<Widget> _pages = const [
    HealthPage(),
    BleScanPage(),
    DebugScreen(),
    DebugTbScreen(),
  ];

  final List<String> _titles = const [
    'Mi Salud',
    'Dispositivos',
    'Depuración',
    'Debug TB',
  ];

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(title: Text(_titles[_index])),
      body: IndexedStack(index: _index, children: _pages),

      bottomNavigationBar: NavigationBar(
        selectedIndex: _index,
        onDestinationSelected: (i) => setState(() => _index = i),

        // Esto ayuda a que se “note” el 4º tab
        labelBehavior: NavigationDestinationLabelBehavior.alwaysShow,

        destinations: const [
          NavigationDestination(
            icon: Icon(Icons.favorite_border_rounded),
            selectedIcon: Icon(Icons.favorite_rounded),
            label: 'Salud',
          ),
          NavigationDestination(
            icon: Icon(Icons.bluetooth_searching_rounded),
            selectedIcon: Icon(Icons.bluetooth_connected_rounded),
            label: 'Conectar',
          ),
          NavigationDestination(
            icon: Icon(Icons.terminal_rounded),
            label: 'Debug',
          ),
          NavigationDestination(
            icon: Icon(Icons.cloud_upload_rounded),
            label: 'Debug TB',
          ),
        ],
      ),
    );
  }
}
