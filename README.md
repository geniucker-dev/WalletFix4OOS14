# Wallet Fix for OxygenOS 14

使OxygenOS 14的**一加12**可以使用Oppo钱包

## 说明

本模块的原理是用一加12的ColorOS全量包中提取Oppo账号应用和OpenID应用，然后替换到OxygenOS 14中，然后修改`ro.product.model`为国内版

## 使用方法

1. 安装Oppo钱包
2. 安装银联可信服务安全组件（如果不需要支付功能可以跳过）
3. 安装本模块
