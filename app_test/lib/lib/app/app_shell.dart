import 'package:flutter/material.dart';
import '../features/Health/health_page.dart'; // Asegúrate de que esta ruta exista
import '../features/bluetooth/ui/ble_scan_page.dart';
import '../features/debug/debug_screen.dart';

class AppShell extends StatefulWidget {
  const AppShell({super.key});

  @override
  State<AppShell> createState() => _AppShellState();
}

class _AppShellState extends State<AppShell> {
  int _index = 0;

  final List<Widget> _pages = const [
    HealthPage(), // Esta página debe contener tu DashboardContent
    BleScanPage(),
    DebugScreen(),
  ];

  final List<String> _titles = const ['Mi Salud', 'Dispositivos', 'Depuración'];

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      // AppBar condicional: Solo mostramos título si es necesario
      appBar: AppBar(
        title: Text(_titles[_index]),
        actions: _index == 0
            ? [
          IconButton(
            icon: const Icon(Icons.notifications_outlined),
            onPressed: () {}, // Futura implementación de notificaciones
          ),
          const SizedBox(width: 8),
        ]
            : null,
      ),
      body: AnimatedSwitcher(
        duration: const Duration(milliseconds: 300),
        child: _pages[_index],
      ),
      bottomNavigationBar: NavigationBar(
        selectedIndex: _index,
        onDestinationSelected: (i) => setState(() => _index = i),
        backgroundColor: Colors.white,
        elevation: 2,
        shadowColor: Colors.black12,
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
        ],
      ),
    );
  }
}