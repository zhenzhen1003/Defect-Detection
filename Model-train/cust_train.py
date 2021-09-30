import paddle
from paddleseg.models.losses import CrossEntropyLoss,DiceLoss,BCELoss
from paddleseg.models import U2Netp
import paddleseg.transforms as T
from paddleseg.datasets import OpticDiscSeg,Dataset
from paddleseg.core import train

train_transforms = [
    # 这里可以加数据增强，例如水平翻转(RandomHorizontalFlip())、随机旋转(RandomRotation())、随机缩放(RandomScaleAspect)等
    T.RandomHorizontalFlip(),
    T.RandomVerticalFlip(),
    T.RandomBlur(0.2),
    T.RandomDistort(),
    T.Normalize()
]
val_transforms = [
    T.RandomHorizontalFlip(),
    T.RandomVerticalFlip(),
    T.RandomBlur(0.2),
    T.RandomDistort(),
    T.Normalize()
]
test_transforms = [
    T.RandomHorizontalFlip(),
    T.RandomVerticalFlip(),
    T.RandomBlur(0.2),
    T.RandomDistort(),
    T.Normalize()
]

dataset_root = 'C:/Users/zhenzhen1003/PaddleSeg/work/newdata'
train_path  = 'C:/Users/zhenzhen1003/PaddleSeg/work/newdata/train_list.txt'
val_path  = 'C:/Users/zhenzhen1003/PaddleSeg/work/newdata/val_list.txt'
test_path  = 'C:/Users/zhenzhen1003/PaddleSeg/work/newdata/test_list.txt'
# 构建训练集
train_dataset = Dataset(
    dataset_root = dataset_root,
    train_path = train_path,
    transforms = train_transforms,
    num_classes = 6,
    mode = 'train'
    )
#验证集
val_dataset = Dataset(
    dataset_root =dataset_root,
    val_path = val_path,
    num_classes = 6,
    transforms = val_transforms,
    mode = 'val'
    )

#测试集
test_dataset = Dataset(
    dataset_root = dataset_root,
    test_path = test_path,
    num_classes = 6,
    transforms = test_transforms,
    mode = 'test'              
    )


#导入模型
"""
    The U^2-Net implementation based on PaddlePaddle.

    The original article refers to
    Xuebin Qin, et, al. "U^2-Net: Going Deeper with Nested U-Structure for Salient Object Detection"
    (https://arxiv.org/abs/2005.09007).

    Args:
        num_classes (int): The unique number of target classes.
        in_ch (int, optional): Input channels. Default: 3.
        pretrained (str, optional): The path or url of pretrained model for fine tuning. Default: None.

"""

# 6分类，输入图片通道为3通道
model = U2Netp(num_classes=6,in_ch=3)



# 设置学习率
base_lr = 0.01
lr = paddle.optimizer.lr.CosineAnnealingDecay(learning_rate=base_lr, T_max=1800, verbose=False)
optimizer = paddle.optimizer.Momentum(lr, parameters=model.parameters(), momentum=0.9, weight_decay=4.0e-5)
losses = {}

# 设置损失函数，设置交叉熵损失函数各缺陷类别权重
losses['types'] = [CrossEntropyLoss(weight = [1., 5.2917, 4.2066, 20.5154, 2.4356, 8.7730])]*7
losses['coef'] = [1]*7


train(
    model=model,
    train_dataset=train_dataset,
    val_dataset=val_dataset,
    optimizer=optimizer,
    save_dir='save_model',
    iters=200,
    batch_size=16,
    save_interval=200,
    log_iters=10,
    num_workers=0,
    losses=losses,
    use_vdl=True
    )