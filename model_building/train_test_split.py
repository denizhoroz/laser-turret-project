"""
Train/Val/Test splitter for YOLO datasets.

Splits a flat YOLO dataset (images/ + labels/) into
train/val/test subdirectories and updates dataset.yaml.

Usage:
    python train_test_split.py
"""

import os
import random
import shutil
import yaml
from pathlib import Path


def get_dataset_folder() -> Path:
    print("\n=== YOLO Dataset Train/Val/Test Splitter ===\n")
    folder = input("Enter dataset folder path (e.g. datasets/dataset1/data/): ").strip()
    path = Path(folder)
    if not path.is_dir():
        print(f"Error: '{folder}' is not a valid directory.")
        exit(1)
    if not (path / "images").is_dir():
        print(f"Error: '{folder}/images/' not found.")
        exit(1)
    if not (path / "labels").is_dir():
        print(f"Error: '{folder}/labels/' not found.")
        exit(1)
    return path


def get_split_ratios() -> tuple[float, float, float]:
    print("\nEnter split percentages (must sum to 100).")
    print("Press Enter for defaults: train=70, val=20, test=10\n")

    raw = input("Train %  [70]: ").strip()
    train = float(raw) if raw else 70.0

    raw = input("Val %    [20]: ").strip()
    val = float(raw) if raw else 20.0

    raw = input("Test %   [10]: ").strip()
    test = float(raw) if raw else 10.0

    total = train + val + test
    if abs(total - 100.0) > 0.01:
        print(f"Error: percentages sum to {total}, must be 100.")
        exit(1)

    return train / 100, val / 100, test / 100


def collect_paired_samples(dataset_path: Path) -> tuple[list[tuple[str, str]], list[str]]:
    """Find image files that have matching label files. Return (pairs, orphan_images)."""
    images_dir = dataset_path / "images"
    labels_dir = dataset_path / "labels"

    image_extensions = {".jpg", ".jpeg", ".png", ".bmp", ".tiff", ".webp"}
    label_files = {p.stem for p in labels_dir.iterdir() if p.suffix == ".txt"}

    pairs = []
    orphan_images = []
    for img_path in sorted(images_dir.iterdir()):
        if img_path.suffix.lower() not in image_extensions:
            continue
        if img_path.stem in label_files:
            label_name = img_path.stem + ".txt"
            pairs.append((img_path.name, label_name))
        else:
            orphan_images.append(img_path.name)

    if orphan_images:
        print(f"\nWarning: {len(orphan_images)} images have no matching label (skipped).")

    orphan_labels = label_files - {Path(p[0]).stem for p in pairs}
    if orphan_labels:
        print(f"Warning: {len(orphan_labels)} labels have no matching image (skipped).")

    print(f"Found {len(pairs)} matched image-label pairs.")
    return pairs, orphan_images


def split_and_move(
    dataset_path: Path,
    pairs: list[tuple[str, str]],
    train_ratio: float,
    val_ratio: float,
    test_ratio: float,
    seed: int = 42,
):
    random.seed(seed)
    random.shuffle(pairs)

    n = len(pairs)
    n_train = int(n * train_ratio)
    n_val = int(n * val_ratio)

    splits = {
        "train": pairs[:n_train],
        "val": pairs[n_train : n_train + n_val],
        "test": pairs[n_train + n_val :],
    }

    images_dir = dataset_path / "images"
    labels_dir = dataset_path / "labels"

    for split_name, split_pairs in splits.items():
        img_dest = images_dir / split_name
        lbl_dest = labels_dir / split_name
        img_dest.mkdir(exist_ok=True)
        lbl_dest.mkdir(exist_ok=True)

        for img_name, lbl_name in split_pairs:
            shutil.move(str(images_dir / img_name), str(img_dest / img_name))
            shutil.move(str(labels_dir / lbl_name), str(lbl_dest / lbl_name))

    print(f"\nSplit complete:")
    print(f"  train: {len(splits['train'])} samples")
    print(f"  val:   {len(splits['val'])} samples")
    print(f"  test:  {len(splits['test'])} samples")

    return splits


def update_yaml(dataset_path: Path):
    yaml_path = dataset_path / "dataset.yaml"
    if not yaml_path.exists():
        print("Warning: dataset.yaml not found, skipping update.")
        return

    with open(yaml_path, "r") as f:
        config = yaml.safe_load(f)

    config["train"] = "images/train"
    config["val"] = "images/val"
    config["test"] = "images/test"

    with open(yaml_path, "w") as f:
        yaml.dump(config, f, default_flow_style=False, sort_keys=False)

    print(f"Updated {yaml_path}")


def main():
    dataset_path = get_dataset_folder()
    train_r, val_r, test_r = get_split_ratios()
    pairs, orphan_images = collect_paired_samples(dataset_path)

    if not pairs:
        print("No matched pairs found. Nothing to split.")
        exit(1)

    print(f"\nWill split {len(pairs)} samples into:")
    print(f"  train: {int(len(pairs) * train_r)}")
    print(f"  val:   {int(len(pairs) * val_r)}")
    print(f"  test:  {len(pairs) - int(len(pairs) * train_r) - int(len(pairs) * val_r)}")

    confirm = input("\nProceed? [Y/n]: ").strip().lower()
    if confirm and confirm != "y":
        print("Aborted.")
        exit(0)

    split_and_move(dataset_path, pairs, train_r, val_r, test_r)
    update_yaml(dataset_path)

    if orphan_images:
        print(f"\n{len(orphan_images)} unlabelled images remain in images/:")
        for name in orphan_images:
            print(f"  {name}")
        remove = input("\nRemove these unlabelled images? [y/N]: ").strip().lower()
        if remove == "y":
            images_dir = dataset_path / "images"
            for name in orphan_images:
                (images_dir / name).unlink()
            print(f"Removed {len(orphan_images)} unlabelled images.")

    print("\nDone.")


if __name__ == "__main__":
    main()
