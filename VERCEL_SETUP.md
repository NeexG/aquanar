# Vercel Deployment Setup - ESP32 Proxy

## Problem
When deploying to Vercel (HTTPS), browsers block HTTP requests to your ESP32 (mixed content error). This proxy solution allows your HTTPS frontend to communicate with your HTTP ESP32 device.

## Solution
A Vercel serverless function (`api/proxy/[...path].js`) acts as a proxy, forwarding HTTPS requests from your frontend to your HTTP ESP32 device.

## Setup Instructions

### 1. Set Environment Variable in Vercel

1. Go to your Vercel project dashboard
2. Navigate to **Settings** → **Environment Variables**
3. Add a new environment variable:
   - **Name:** `ESP32_IP`
   - **Value:** Your ESP32's IP address (e.g., `192.168.0.111`)
   - **Environment:** Production, Preview, Development (select all)

### 2. Deploy to Vercel

```bash
# If not already deployed
vercel

# Or push to your connected Git repository
git push origin main
```

### 3. How It Works

**Production (Vercel):**
- Frontend makes HTTPS request: `https://aquanar.vercel.app/api/proxy/status`
- Vercel serverless function proxies to: `http://192.168.0.111/api/status`
- No mixed content error! ✅

**Development (Local):**
- Frontend makes HTTP request directly: `http://192.168.0.111/api/status`
- Works normally on localhost

### 4. API Endpoint Mapping

| Frontend Request | Proxy Forwards To |
|-----------------|-------------------|
| `/api/proxy/status` | `http://ESP32_IP/api/status` |
| `/api/proxy/control` | `http://ESP32_IP/api/control` |
| `/api/proxy/species` | `http://ESP32_IP/api/species` |
| `/api/proxy/wifi` | `http://ESP32_IP/api/wifi` |
| `/api/proxy/ping` | `http://ESP32_IP/api/ping` |

### 5. Important Notes

⚠️ **ESP32 Must Be Accessible from Internet**
- The Vercel serverless function runs on Vercel's servers
- It needs to reach your ESP32 over the internet
- Your ESP32's local IP (192.168.x.x) is **NOT accessible** from the internet

**Solutions:**

**Option A: Use ngrok (Recommended for Testing)**
```bash
# Install ngrok
npm install -g ngrok

# Create tunnel to ESP32
ngrok http 192.168.0.111:80

# Use the ngrok HTTPS URL in Vercel environment variable
# ESP32_IP = "your-ngrok-url.ngrok.io" (without http://)
```

**Option B: Port Forwarding (For Permanent Setup)**
1. Configure your router to forward port 80 to your ESP32
2. Use your public IP or domain name
3. Set `ESP32_IP` to your public IP/domain

**Option C: Use a Cloud Proxy Service**
- Services like Cloudflare Tunnel, Tailscale, etc.

### 6. Testing

1. **Test locally:**
   ```bash
   npm run dev
   # Should connect directly to ESP32
   ```

2. **Test on Vercel:**
   - Deploy and check browser console
   - Should use proxy endpoints
   - No mixed content errors

### 7. Troubleshooting

**Error: "Cannot connect to ESP32"**
- Check `ESP32_IP` environment variable is set correctly
- Verify ESP32 is accessible from the internet
- Check ESP32 Serial Monitor for connection attempts

**Error: "Gateway Timeout"**
- ESP32 might be offline or unreachable
- Check network connectivity
- Verify ESP32 IP address

**Still getting mixed content errors?**
- Clear browser cache
- Check browser console for actual request URLs
- Verify the proxy is being used (check Network tab)

### 8. Alternative: Development vs Production Config

If you want different behavior in development vs production, you can also set environment variables:

```javascript
// In your code
const ESP32_IP = import.meta.env.VITE_ESP32_IP || '192.168.0.111';
```

Then set `VITE_ESP32_IP` in Vercel environment variables.

---

## Quick Start Checklist

- [ ] Created `api/proxy/[...path].js` file
- [ ] Set `ESP32_IP` environment variable in Vercel
- [ ] Made ESP32 accessible from internet (ngrok/port forwarding)
- [ ] Deployed to Vercel
- [ ] Tested connection from deployed site

---

## Need Help?

If your ESP32 is on a local network and not accessible from the internet, you have two options:

1. **Use ngrok for testing** (easiest, but requires ngrok to be running)
2. **Deploy a backend service** that can reach your local ESP32 (more complex)

For production use, consider setting up proper port forwarding or using a cloud-based IoT platform.

