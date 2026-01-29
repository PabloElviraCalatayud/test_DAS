import 'package:flutter/material.dart';

import '../../app/app_shell.dart';
import '../../core/utils/onboarding_storage.dart';
import '../../shared/widgets/buttons/primary_button.dart';
import 'onboarding_content.dart';
import 'package:flutter_svg/flutter_svg.dart';

class OnboardingPage extends StatefulWidget {
  const OnboardingPage({super.key});

  @override
  State<OnboardingPage> createState() => _OnboardingPageState();
}

class _OnboardingPageState extends State<OnboardingPage> {
  final PageController _controller = PageController();
  int _currentPage = 0;

  final List<Map<String, String>> onboardingData = [
    {
      "image": "assets/illustrations/connected_world.svg",
      "title": "Conecta tu DAS",
      "description":
      "Empareja tu dispositivo con el DAS para soñar como nunca has soñado.",
    },
    {
      "image": "assets/illustrations/heart.svg",
      "title": "Monitorea tu pulso y O2",
      "description":
      "Visualiza en tiempo real tus pulsaciones y oxígeno en sangre",
    },
    {
      "image": "assets/illustrations/moon.svg",
      "title": "Detecta tus episodios de apnea",
      "description":
      "Descubre tus tendencias, mejora tu descanso y evita percances con DDAS.",
    },
  ];

  Future<void> _finishOnboarding() async {
    await OnboardingStorage.setSeen();

    if (!mounted) return;

    Navigator.of(context).pushReplacement(
      MaterialPageRoute(
        builder: (_) => const AppShell(),
      ),
    );
  }

  void _nextPage() {
    if (_currentPage == onboardingData.length - 1) {
      _finishOnboarding();
    } else {
      _controller.nextPage(
        duration: const Duration(milliseconds: 400),
        curve: Curves.easeInOut,
      );
    }
  }

  void _previousPage() {
    if (_currentPage > 0) {
      _controller.previousPage(
        duration: const Duration(milliseconds: 400),
        curve: Curves.easeInOut,
      );
    }
  }

  void _skip() {
    _finishOnboarding();
  }

  @override
  Widget build(BuildContext context) {
    final theme = Theme.of(context);

    return Scaffold(
      backgroundColor: Colors.white,
      body: SafeArea(
        child: Column(
          children: [
            // Botón Saltar minimalista
            Align(
              alignment: Alignment.topRight,
              child: Padding(
                padding: const EdgeInsets.all(16.0),
                child: TextButton(
                  onPressed: _skip,
                  child: Text("Saltar", style: TextStyle(color: theme.colorScheme.primary)),
                ),
              ),
            ),

            Expanded(
              child: PageView.builder(
                controller: _controller,
                itemCount: onboardingData.length,
                onPageChanged: (v) => setState(() => _currentPage = v),
                itemBuilder: (ctx, idx) => _buildPageContent(onboardingData[idx]),
              ),
            ),

            // Indicadores y Botones
            Padding(
              padding: const EdgeInsets.all(32.0),
              child: Column(
                children: [
                  // Indicador de página (Puntos)
                  Row(
                    mainAxisAlignment: MainAxisAlignment.center,
                    children: List.generate(
                      onboardingData.length,
                          (index) => AnimatedContainer(
                        duration: const Duration(milliseconds: 300),
                        margin: const EdgeInsets.symmetric(horizontal: 4),
                        height: 6,
                        width: _currentPage == index ? 24 : 6,
                        decoration: BoxDecoration(
                          color: _currentPage == index ? theme.colorScheme.primary : const Color(0xFFE0E0E0),
                          borderRadius: BorderRadius.circular(4),
                        ),
                      ),
                    ),
                  ),
                  const SizedBox(height: 32),
                  // Botón Principal Grande
                  SizedBox(
                    width: double.infinity,
                    height: 56,
                    child: FilledButton(
                      style: FilledButton.styleFrom(
                        shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(16)),
                        backgroundColor: theme.colorScheme.primary,
                      ),
                      onPressed: _nextPage,
                      child: Text(
                        _currentPage == onboardingData.length - 1 ? "Comenzar" : "Siguiente",
                        style: const TextStyle(fontSize: 18, fontWeight: FontWeight.bold),
                      ),
                    ),
                  ),
                ],
              ),
            ),
          ],
        ),
      ),
    );
  }

  Widget _buildPageContent(Map<String, String> data) {
    return Padding(
      padding: const EdgeInsets.all(24.0),
      child: Column(
        mainAxisAlignment: MainAxisAlignment.center,
        children: [
          SvgPicture.asset(
            data["image"]!,
            height: 280, // Imagen más grande
          ),
          const SizedBox(height: 40),
          Text(
            data["title"]!,
            textAlign: TextAlign.center,
            style: const TextStyle(
              fontSize: 28,
              fontWeight: FontWeight.w800,
              letterSpacing: -0.5,
              color: Color(0xFF1A1C1E),
            ),
          ),
          const SizedBox(height: 16),
          Text(
            data["description"]!,
            textAlign: TextAlign.center,
            style: const TextStyle(
              fontSize: 16,
              height: 1.5,
              color: Color(0xFF757575),
            ),
          ),
        ],
      ),
    );
  }
}