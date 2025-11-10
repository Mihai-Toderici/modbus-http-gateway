# C++ Modbus TCP to REST API Gateway

A lightweight, high-performance C++ application that acts as a protocol gateway, simultaneously exposing industrial Modbus TCP devices (on port 502) to a modern RESTful API (on port 8080).

## Prerequisites
- Visual Studio 2022
- vcpkg
- C++ Compiler


## How to Use

Once the application is running, you can test both servers.

### 1.Test the HTTP API(Read and Write)

The server runs at `http://localhost:8080`.

* **Read (Browser):**
    Open `http://localhost:8080/api/register/0` in your web browser.

* **Write (PowerShell):**
    Open your terminal and invoke a post method to set the value to `999`.
    ```powershell
    Invoke-RestMethod -Method Post -Uri "http://localhost:8080/api/register/0?value=999"
    ```

* **Verify (Browser):**
    Refresh your browser. It will now show `999`.

### 2. Test the Modbus Server (Read and Write)

The server runs on port `502`.
Use a Modbus client like Modbus Poll (https://www.modbustools.com/modbus_poll.html) and write to the first register.
Refresh your web browser (`http://localhost:8080/api/register/0`) one last time and you should see the value update
