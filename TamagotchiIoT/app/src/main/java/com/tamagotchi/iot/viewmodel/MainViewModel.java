package com.tamagotchi.iot.viewmodel;

import android.app.Application;
import android.bluetooth.BluetoothDevice;
import androidx.lifecycle.AndroidViewModel;
import androidx.lifecycle.LiveData;
import com.tamagotchi.iot.bluetooth.BluetoothSPPManager;
import com.tamagotchi.iot.bluetooth.ConnectionState;

public class MainViewModel extends AndroidViewModel {

    private final BluetoothSPPManager bluetoothManager;
    
    // LiveData que el Activity observará
    public final LiveData<ConnectionState> connectionState;
    public final LiveData<String> incomingMessage;

    public MainViewModel(Application application) {
        super(application);
        // 1. Instancia del Manager
        bluetoothManager = new BluetoothSPPManager();
        
        // 2. Exponer LiveData del Manager
        connectionState = bluetoothManager.connectionState;
        incomingMessage = bluetoothManager.incomingMessage;
    }

    // Lógica de Control (Invocada por la Activity)

    // 3. Función para iniciar la conexión
    public void connectToDevice(BluetoothDevice device) {
        // El Manager espera un String address
        if (device != null) {
            bluetoothManager.connect(device.getAddress());
        }
    }

    public void disconnect() {
        bluetoothManager.disconnect();
    }
    
    // 4. Funciones de Comando
    public void sendFeedCommand() {
        android.util.Log.d("DEBUG_TAG", "ViewModel: Comando 'F' recibido. Pasando al Bluetooth Manager.");
        bluetoothManager.sendCommand("F");
    }

    public void sendPlayCommand() {
        android.util.Log.d("DEBUG_TAG", "ViewModel: Comando 'P' recibido. Pasando al Bluetooth Manager.");
        bluetoothManager.sendCommand("P");
    }

    public void sendSleepCommand() {
        android.util.Log.d("DEBUG_TAG", "ViewModel: Comando 'S' recibido. Pasando al Bluetooth Manager.");
        bluetoothManager.sendCommand("S");
    }

    public void sendCheatCommand(int hunger, int play, int sleep) {
        // Protocolo: #H,F,E
        String command = "#" + hunger + "," + play + "," + sleep + "\n";
        bluetoothManager.sendCommand(command);
    }
    
    // Limpieza de recursos
    @Override
    protected void onCleared() {
        super.onCleared();
        // Cierra la conexión Bluetooth cuando el ViewModel ya no se use
        bluetoothManager.disconnect(); 
    }
}
