# Quick Setup: Make ESP32 Accessible from Vercel using ngrok

## Problem
Your ESP32 is on a local network (`192.168.0.111`) which Vercel servers cannot reach. You need to make it accessible from the internet.

## Solution: Use ngrok (Easiest for Testing)

### Step 1: Install ngrok

**Windows:**
1. Download from: https://ngrok.com/download
2. Extract `ngrok.exe` to a folder (e.g., `C:\ngrok\`)
3. Add to PATH or use full path

**Or use npm:**
```bash
npm install -g ngrok
```

### Step 2: Start ngrok Tunnel

1. Make sure your ESP32 is running and accessible at `http://192.168.0.111`
2. Open terminal/command prompt
3. Run:
   ```bash
   ngrok http 192.168.0.111:80
   ```

4. You'll see output like:
   ```
   Forwarding   https://abc123.ngrok-free.app -> http://localhost:192.168.0.111:80
   ```

5. **Copy the HTTPS URL** (e.g., `abc123.ngrok-free.app`)

### Step 3: Update Vercel Environment Variable

1. Go to Vercel Dashboard → Your Project → **Settings** → **Environment Variables**
2. Find or create: `ESP32_IP`
3. Update the value to: `abc123.ngrok-free.app` (use your ngrok URL, **without** `http://`)
4. Save and redeploy

### Step 4: Test

1. Visit your Vercel site
2. The proxy should now be able to reach your ESP32 through ngrok
3. Check Vercel function logs to see if requests are successful

## Important Notes

⚠️ **ngrok Free Tier Limitations:**
- URL changes every time you restart ngrok (unless you have a paid plan)
- You'll need to update `ESP32_IP` in Vercel each time
- Free tier has connection limits

✅ **For Permanent Solution:**
- Use ngrok paid plan (static domain)
- Or set up port forwarding on your router
- Or use a cloud-based IoT platform

## Alternative: Port Forwarding (Permanent)

If you want a permanent solution:

1. **Get your public IP:**
   - Visit: https://whatismyipaddress.com/
   - Note your public IP address

2. **Configure Router:**
   - Log into your router admin panel
   - Set up port forwarding: External Port 80 → Internal IP 192.168.0.111:80
   - Save settings

3. **Update Vercel:**
   - Set `ESP32_IP` to your public IP address
   - Or use a domain name pointing to your public IP

4. **Security Note:**
   - Exposing your ESP32 directly to the internet has security risks
   - Consider adding authentication or using a VPN

## Troubleshooting

**ngrok not working?**
- Make sure ESP32 is running
- Check ngrok is forwarding to correct IP:port
- Verify ESP32 responds to `http://192.168.0.111/api/ping` locally

**Still getting timeout?**
- Check ngrok dashboard: https://dashboard.ngrok.com/
- Verify ngrok tunnel is active
- Check Vercel function logs for detailed errors

**URL keeps changing?**
- Use ngrok paid plan for static domain
- Or set up a script to auto-update Vercel env variable
- Or use port forwarding instead

---

## Quick Command Reference

```bash
# Start ngrok tunnel
ngrok http 192.168.0.111:80

# Check ngrok status
curl http://localhost:4040/api/tunnels

# Test ESP32 locally
curl http://192.168.0.111/api/ping

# Test through ngrok (replace with your ngrok URL)
curl https://abc123.ngrok-free.app/api/ping
```

