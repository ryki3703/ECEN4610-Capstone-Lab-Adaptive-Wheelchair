import 'package:flutter/material.dart';

void main() {
  runApp(const WheelchairApp());
}

class WheelchairApp extends StatelessWidget {
  const WheelchairApp({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      debugShowCheckedModeBanner: false,
      title: 'Wheelchair Remote',
      theme: ThemeData(
        brightness: Brightness.dark,
        useMaterial3: true,
      ),
      home: const CameraScreen(),
    );
  }
}

class CameraScreen extends StatelessWidget {
  const CameraScreen({super.key});

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('Wheelchair Remote'),
        centerTitle: true,
      ),
      body: Center(
        child: Column(
          mainAxisAlignment: MainAxisAlignment.center,
          children: [
            Container(
              width: 600,
              height: 400,
              color: Colors.black,
              child: const Center(
                child: Text(
                  'ESP32-CAM',
                  style: TextStyle(fontSize: 24),
                ),
              ),
            ),
            const SizedBox(height: 20),
            const Text(
              'Camera disconnected',
              style: TextStyle(
                color: Colors.red,
                fontSize: 18,
              ),
            ),
          ],
        ),
      ),
    );
  }
}