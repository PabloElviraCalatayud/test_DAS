import 'package:flutter/material.dart';

import '../../app/app_shell.dart';
import '../../core/utils/onboarding_storage.dart';
import '../../shared/widgets/buttons/primary_button.dart';
import 'onboarding_content.dart';

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
    final colorScheme = Theme.of(context).colorScheme;

    return Scaffold(
      backgroundColor: Theme.of(context).scaffoldBackgroundColor,
      body: SafeArea(
        child: Stack(
          children: [
            Column(
              children: [
                Expanded(
                  flex: 4,
                  child: PageView.builder(
                    controller: _controller,
                    itemCount: onboardingData.length,
                    onPageChanged: (value) {
                      setState(() {
                        _currentPage = value;
                      });
                    },
                    itemBuilder: (context, index) => OnboardingContent(
                      image: onboardingData[index]["image"]!,
                      title: onboardingData[index]["title"]!,
                      description:
                      onboardingData[index]["description"]!,
                    ),
                  ),
                ),
                const SizedBox(height: 20),
                Row(
                  mainAxisAlignment: MainAxisAlignment.center,
                  children: List.generate(
                    onboardingData.length,
                        (index) => AnimatedContainer(
                      duration: const Duration(milliseconds: 300),
                      margin: const EdgeInsets.symmetric(horizontal: 6),
                      height: 8,
                      width: _currentPage == index ? 24 : 8,
                      decoration: BoxDecoration(
                        color: _currentPage == index
                            ? colorScheme.primary
                            : colorScheme.onSurface
                            .withValues(alpha: 0.3),
                        borderRadius: BorderRadius.circular(8),
                      ),
                    ),
                  ),
                ),
                const SizedBox(height: 32),
                Padding(
                  padding: const EdgeInsets.symmetric(
                    horizontal: 24,
                    vertical: 24,
                  ),
                  child: Row(
                    children: [
                      Expanded(
                        child: PrimaryButton(
                          text: "Atrás",
                          onPressed: _previousPage,
                        ),
                      ),
                      const SizedBox(width: 16),
                      Expanded(
                        child: PrimaryButton(
                          text: _currentPage ==
                              onboardingData.length - 1
                              ? "Comenzar"
                              : "Siguiente",
                          onPressed: _nextPage,
                        ),
                      ),
                    ],
                  ),
                ),
              ],
            ),
            Positioned(
              top: 12,
              right: 16,
              child: TextButton(
                onPressed: _skip,
                child: Text(
                  "Saltar",
                  style: TextStyle(
                    color: colorScheme.primary,
                    fontWeight: FontWeight.bold,
                  ),
                ),
              ),
            ),
          ],
        ),
      ),
    );
  }
}
