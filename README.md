# Wallet Fix for OxygenOS 14

使OxygenOS 14的**一加12**可以使用Oppo钱包，若使用其他设备，请将安装包中的`system.prop`中的`ro.product.model`字段改为对应机型的国行机型编号

## 说明

本模块使用从 一加12 ColorOS 的全量OTA包中提取的 账号中心 和 OpenID 替换系统中的对应应用，然后修改`ro.product.model`为国内版

## 使用方法

1. 安装Oppo钱包
2. 安装银联可信服务安全组件（如果不需要支付功能可以跳过）
3. 安装本模块
