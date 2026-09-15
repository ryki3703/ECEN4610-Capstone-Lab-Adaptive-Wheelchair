import 'package:flutter/material.dart';
import 'package:mjpeg_view/mjpeg_view.dart';

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

  static const String cameraUrl = 'http://192.168.4.1:81/stream';

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('Wheelchair Remote'),
        centerTitle: true,
      ),
      body: Center(
        child: Padding(
          padding: const EdgeInsets.all(16),
          child: AspectRatio(
            aspectRatio: 4 / 3,
            child: ClipRRect(
              borderRadius: BorderRadius.circular(12),
              child: MjpegView(
                uri: cameraUrl,
                fit: BoxFit.contain,
                fps: 20,
                loadingWidget: (context) {
                  return const Center(
                    child: CircularProgressIndicator(),
                  );
                },
                errorWidget: (context) {
                  return const Center(
                    child: Text(
                      'Camera connection failed',
                      textAlign: TextAlign.center,
                    ),
                  );
                },
              ),
            ),
          ),
        ),
      ),
    );
  }
}