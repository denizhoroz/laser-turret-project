# Jetson deployment

One-time bring-up of `jetson_mission_codes` on Jetson Nano dev kit (JetPack 4.6+),
booted unattended inside the turret.

## 1. Network: Jetson micro-USB ↔ Windows laptop

Jetson dev kit's micro-USB exposes RNDIS gadget. Laptop sees a virtual NIC.

| Side    | IP              |
|---------|-----------------|
| Jetson  | 192.168.55.1    |
| Laptop  | 192.168.55.100  |

On Windows: plug cable in once with Jetson booted. Device Manager → "Remote
NDIS Compatible Device" appears, network adapter gets `192.168.55.100`. If a
different address is assigned, run `ipconfig` and update `GS_HOST` in `.env`
accordingly. Allow inbound TCP 9001 on the RNDIS adapter in Windows Firewall.

## 2. Copy code

From the dev box:

```bash
rsync -av --delete \
  jetson_mission_codes/ \
  jetson@192.168.55.1:/home/jetson/laser-turret/jetson_mission_codes/
```

Models (`*.pt`) must end up under `src/models/` per `config.py`.

## 3. Python venv

On Jetson:

```bash
cd /home/jetson/laser-turret/jetson_mission_codes
python3 -m venv venv
source venv/bin/activate
pip install --upgrade pip
pip install -r requirements-jetson.txt
```

### torch (Jetson-specific)

PyPI torch will NOT work. Install NVIDIA's prebuilt wheel matching the L4T
version. Check version with `cat /etc/nv_tegra_release`, then follow
<https://forums.developer.nvidia.com/t/pytorch-for-jetson/72048>.

Example (JetPack 4.6, Python 3.8, L4T 32.7):

```bash
wget https://nvidia.box.com/shared/static/<wheel-id>.whl -O torch.whl
pip install torch.whl
```

## 4. Serial access

```bash
sudo usermod -aG dialout,video jetson
# logout/login OR reboot to apply group membership
```

Verify Arduino enumerates: `ls /dev/ttyACM*` with MEGA plugged in.

## 5. Env file

```bash
cd /home/jetson/laser-turret/jetson_mission_codes
cp .env.example .env
# edit if Windows assigned a different IP than 192.168.55.100
```

## 6. systemd unit

```bash
sudo cp deploy/laser-turret.service /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl enable --now laser-turret.service
```

## 7. Verify

```bash
# Live logs:
journalctl -u laser-turret -f

# Service state:
systemctl status laser-turret

# Manual restart:
sudo systemctl restart laser-turret
```

On the laptop, open the Ground Station. Connection pill should turn green
within ~10 s of Jetson finishing boot.

## Recovery

| Symptom                          | Action                                              |
|----------------------------------|-----------------------------------------------------|
| GS never connects                | `ipconfig` on laptop → confirm RNDIS IP; update .env|
| Service flaps in journalctl      | Check Arduino USB; `ls /dev/ttyACM*`                |
| Model missing                    | Re-rsync `src/models/`                              |
| Camera unavailable               | `ls /dev/video*`; reseat ribbon                     |
| Need to disable autostart        | `sudo systemctl disable --now laser-turret`         |
