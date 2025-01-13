# Binc Trader

**Binc Trader** is a Linux-based trading application designed to work seamlessly with the **Deriv** broker. It offers a comprehensive suite of tools for traders to manage their portfolios, analyze market trends, and execute trades efficiently.

## Features
- **User-Friendly Interface**: Intuitive and easy-to-navigate user interface.
- **Market Analysis**: Real-time market data and advanced analytical tools.
- **Trade Execution**: Swift and reliable trade execution with Deriv integration.
- **Portfolio Management**: Track and manage your investments effortlessly.
- **Customizable**: Personalize the app to suit your trading style and preferences.
- **Security**: Robust security features to protect your data and transactions.

## Dependencies

Ensure you have the following dependencies installed on your system:

- `webkitgtk-6.0`
- `epoxy`
- `sqlite3`
- `json-glib-1.0`
- `libsoup-3.0`
- `libsecret-1`

You can install these dependencies using your distribution's package manager. For example, on Ubuntu, you can use:

```sh
sudo apt install libwebkitgtk-6.0-dev libepoxy-dev libsqlite3-dev libjson-glib-dev libsoup-3.0-dev libsecret-1-dev
```
### Steps
1. Clone the repository:
```sh
git clone https://github.com/binclab/trader.git
```
2. Navigate to the project directory:
```sh
cd trader
```
3. Build the application: 
```sh
make
```
4. Run the application:
```sh
./build/binctrader
```