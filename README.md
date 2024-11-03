# Wallet Fix for OxygenOS 14

使OxygenOS 14的**一加12**可以使用Oppo钱包，若使用其他设备，请在安装完成后创建`/data/adb/walletfix/spoof_vars`文件，文件内容为`MODEL=你的设备型号`

## 说明

本模块使用从 一加12 ColorOS 的全量OTA包中提取的 账号中心 和 OpenID 替换系统中的对应应用，然后对钱包应用伪装为国行系统

## 使用方法

1. 安装Oppo钱包
2. 安装银联可信服务安全组件（如果不需要支付功能可以跳过）
3. 安装本模块

## 已知问题

1. eid 不工作

## License

[GPL-3.0](LICENSE)

## Acknowledgements

- [PlayIntegrityFix](https://github.com/chiteroman/PlayIntegrityFix)
- [TrickyStore](https://github.com/5ec1cff/TrickyStore/tree/master)
