package com.tamagotchi.iot.bluetooth;

import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothSocket;
import androidx.lifecycle.LiveData;
import androidx.lifecycle.MutableLiveData;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.UUID;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public class BluetoothSPPManager {
    // UUID estándar para SPP (Serial Port Profile)
    private static final UUID SPP_UUID = UUID.fromString("00001101-0000-1000-8000-00805F9B34FB");
    
    private final BluetoothAdapter bluetoothAdapter;
    private final ExecutorService executorService;
    
    // LiveData para el estado de la conexión
    private final MutableLiveData<ConnectionState> _connectionState = new MutableLiveData<>(ConnectionState.DISCONNECTED);
    public LiveData<ConnectionState> connectionState = _connectionState;

    // LiveData para mensajes entrantes
    private final MutableLiveData<String> _incomingMessage = new MutableLiveData<>();
    public LiveData<String> incomingMessage = _incomingMessage;

    private ConnectThread connectThread;

    public BluetoothSPPManager() {
        bluetoothAdapter = BluetoothAdapter.getDefaultAdapter();
        // ExecutorService para manejar hilos en background
        executorService = Executors.newSingleThreadExecutor();
    }

    public void connect(String deviceAddress) {
        if (bluetoothAdapter == null || deviceAddress == null) return;
        
        try {
            BluetoothDevice device = bluetoothAdapter.getRemoteDevice(deviceAddress);
            
            // 1. Cancelar cualquier conexión existente
            if (connectThread != null) {
                android.util.Log.d("DEBUG_TAG", "Manager: Cancelando hilo anterior.");
                connectThread.cancel();
                connectThread = null;
            }

            // 2. Crear un nuevo hilo de conexión
            connectThread = new ConnectThread(device);
            
            // 3. Iniciar el nuevo hilo
            android.util.Log.d("DEBUG_TAG", "Manager: Iniciando ConnectThread con el dispositivo: " + device.getAddress());
            connectThread.start();
            
        } catch (IllegalArgumentException e) {
            android.util.Log.e("DEBUG_TAG", "Manager: Error al obtener dispositivo remoto: " + e.getMessage());
            _connectionState.postValue(ConnectionState.ERROR);
        }
    }

    public void disconnect() {
        if (connectThread != null) {
            connectThread.cancel();
            connectThread = null;
        }
    }

    public void sendCommand(String command) {
        if (connectThread != null) {
            // Enviar en un hilo separado para no bloquear UI si el buffer está lleno (aunque write suele ser rápido)
            executorService.execute(() -> connectThread.write(command.getBytes()));
        } else {
            android.util.Log.e("DEBUG_TAG", "Manager: Error - connectThread es NULL. No hay conexión activa.");
        }
    }

    // Hilo que maneja la conexión y la comunicación
    private class ConnectThread extends Thread {
        private final BluetoothSocket mmSocket;
        private final BluetoothDevice mmDevice;
        private InputStream mmInStream;
        private OutputStream mmOutStream;
        private volatile boolean isRunning = true;

        public ConnectThread(BluetoothDevice device) {
            mmDevice = device;
            BluetoothSocket tmp = null;
            try {
                tmp = device.createRfcommSocketToServiceRecord(SPP_UUID);
            } catch (IOException e) {
                _connectionState.postValue(ConnectionState.ERROR);
            }
            mmSocket = tmp;
        }

        @Override
        public void run() {
            if (mmSocket == null) {
                _connectionState.postValue(ConnectionState.ERROR);
                return;
            }

            _connectionState.postValue(ConnectionState.CONNECTING);
            
            // Cancelar descubrimiento para mejorar rendimiento de conexión
            if (bluetoothAdapter.isDiscovering()) {
                bluetoothAdapter.cancelDiscovery();
            }

            try {
                mmSocket.connect();
            } catch (IOException connectException) {
                try {
                    mmSocket.close();
                } catch (IOException closeException) { }
                _connectionState.postValue(ConnectionState.ERROR);
                return;
            }

            // Conexión Exitosa
            _connectionState.postValue(ConnectionState.CONNECTED);

            // Obtener Streams
            try {
                mmInStream = mmSocket.getInputStream();
                mmOutStream = mmSocket.getOutputStream();
            } catch (IOException e) {
                _connectionState.postValue(ConnectionState.ERROR);
                return;
            }

            // Bucle de Lectura
            byte[] buffer = new byte[1024];
            int bytes;

            while (isRunning) {
                try {
                    // Leer del InputStream (Bloqueante)
                    bytes = mmInStream.read(buffer);
                    if (bytes > 0) {
                        String readMessage = new String(buffer, 0, bytes);
                        // PostValue asegura que se actualice en el hilo principal
                        _incomingMessage.postValue(readMessage);
                    }
                } catch (IOException e) {
                    // Se perdió la conexión
                    _connectionState.postValue(ConnectionState.DISCONNECTED);
                    break;
                }
            }
        }

        public void write(byte[] bytes) {
            try {
                if (mmOutStream != null) {
                    android.util.Log.d("DEBUG_TAG", "Manager: Intentando escribir: " + new String(bytes) + " al socket.");
                    mmOutStream.write(bytes);
                    mmOutStream.flush(); // Asegurar envío inmediato
                } else {
                    android.util.Log.e("DEBUG_TAG", "Manager: mmOutStream es NULL. No se puede escribir.");
                }
            } catch (IOException e) {
                android.util.Log.e("DEBUG_TAG", "Manager: Error al escribir en el socket: " + e.getMessage());
            }
        }

        public void cancel() {
            isRunning = false;
            try {
                if (mmSocket != null) {
                    mmSocket.close();
                }
            } catch (IOException e) { }
            _connectionState.postValue(ConnectionState.DISCONNECTED);
        }
    }
}
