注意事项

目前测试阶段是键值对的raft
后续需要修改

1. Raft 层
尽量通用：
    选举
    心跳
    日志复制
    commit 推进
    apply 调度

2. Command/Log Entry 编码层
负责定义：
    日志里放什么内容
    怎么序列化 / 反序列化
    怎么做版本兼容

