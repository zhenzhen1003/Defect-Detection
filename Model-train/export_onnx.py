# -!- coding: utf-8 -!-
import os
import time
import paddle
from paddleseg.models import U2Netp


model = U2Netp(num_classes=6,in_ch=3)
model.set_dict(paddle.load('model.pdparams'))
# 将模型设置为评估状态
model.eval()
# 定义输入数据
input_spec = paddle.static.InputSpec(shape=[1, 3, 240, 240], dtype='float32', name='image')
# ONNX模型导出
paddle.onnx.export(model, 'u2netp', input_spec=[input_spec], opset_version=11)