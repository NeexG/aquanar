/**
 * Vercel Serverless Function - ESP32 API Proxy
 * 
 * This function proxies requests from the HTTPS frontend to the HTTP ESP32 device.
 * This solves the mixed content security issue.
 * 
 * Usage:
 * - GET /api/proxy/status -> proxies to ESP32 /api/status
 * - POST /api/proxy/control -> proxies to ESP32 /api/control
 * - etc.
 * 
 * Environment Variables (set in Vercel dashboard):
 * - ESP32_IP: The IP address of your ESP32 (e.g., "192.168.0.111")
 */

/**
 * Vercel Serverless Function - ESP32 API Proxy
 * Handles all HTTP methods (GET, POST, PUT, DELETE, etc.)
 */
// Helper function to check if IP is a local/private IP
function isLocalIP(ip) {
  if (!ip) return false;
  
  // Remove protocol if present
  ip = ip.replace(/^https?:\/\//, '').split(':')[0];
  
  // Check for localhost
  if (ip === 'localhost' || ip === '127.0.0.1') return true;
  
  // Check for private IP ranges
  const privateRanges = [
    /^10\./,           // 10.0.0.0/8
    /^172\.(1[6-9]|2[0-9]|3[01])\./, // 172.16.0.0/12
    /^192\.168\./,     // 192.168.0.0/16
    /^169\.254\./,     // 169.254.0.0/16 (link-local)
  ];
  
  return privateRanges.some(range => range.test(ip));
}

export default async function handler(req, res) {
  // Get ESP32 IP from environment variable
  const ESP32_IP = process.env.ESP32_IP || '192.168.0.111';
  
  // Check if it's a local IP (won't work from Vercel servers)
  if (isLocalIP(ESP32_IP)) {
    console.error(`[Proxy] ERROR: ESP32_IP is set to local IP: ${ESP32_IP}`);
    console.error('[Proxy] Local IPs are not accessible from Vercel servers.');
    console.error('[Proxy] Solutions: Use ngrok, port forwarding, or a public IP/domain.');
    
    return res.status(503).json({
      success: false,
      error: 'Configuration Error',
      message: `ESP32 IP (${ESP32_IP}) is a local network address and cannot be reached from Vercel servers.`,
      solution: 'Use ngrok (ngrok http 192.168.0.111:80) or set up port forwarding, then update ESP32_IP environment variable in Vercel with the public URL.',
      help: 'See VERCEL_SETUP.md for detailed instructions'
    });
  }
  
  const ESP32_BASE_URL = ESP32_IP.startsWith('http') ? ESP32_IP : `http://${ESP32_IP}`;
  
  // Get the path from the request (everything after /api/proxy/)
  const path = req.query.path || [];
  const apiPath = Array.isArray(path) ? path.join('/') : path;
  
  // Construct the full ESP32 URL
  const targetUrl = `${ESP32_BASE_URL}/api/${apiPath}`;
  
  // Set CORS headers
  res.setHeader('Access-Control-Allow-Origin', '*');
  res.setHeader('Access-Control-Allow-Methods', 'GET, POST, PUT, DELETE, OPTIONS, PATCH');
  res.setHeader('Access-Control-Allow-Headers', 'Content-Type, Authorization');
  
  // Handle preflight OPTIONS request
  if (req.method === 'OPTIONS') {
    return res.status(200).end();
  }
  
  // Log for debugging
  console.log(`[Proxy] ${req.method} ${req.url} -> ${targetUrl}`);
  console.log(`[Proxy] ESP32_IP from env: ${ESP32_IP}`);
  
  try {
    // Parse request body if present
    let requestBody = null;
    if (req.method !== 'GET' && req.method !== 'HEAD') {
      // Handle different body formats
      if (req.body) {
        if (typeof req.body === 'string') {
          requestBody = req.body;
        } else if (typeof req.body === 'object') {
          requestBody = JSON.stringify(req.body);
        }
      }
    }
    
    // Create timeout controller (compatible with Node.js 18+)
    const controller = new AbortController();
    const timeoutId = setTimeout(() => controller.abort(), 15000); // 15 second timeout (increased)
    
    // Forward the request to ESP32
    const fetchOptions = {
      method: req.method,
      headers: {
        'Content-Type': 'application/json',
      },
      signal: controller.signal
    };
    
    if (requestBody) {
      fetchOptions.body = requestBody;
    }
    
    const response = await fetch(targetUrl, fetchOptions);
    clearTimeout(timeoutId);
    
    // Get response data
    const contentType = response.headers.get('content-type');
    let data;
    
    if (contentType && contentType.includes('application/json')) {
      data = await response.json();
    } else {
      data = await response.text();
    }
    
    // Forward the response with proper status code
    return res.status(response.status).json(data);
    
  } catch (error) {
    console.error('[Proxy] Error details:', {
      name: error.name,
      message: error.message,
      code: error.code,
      stack: error.stack
    });
    
    // Handle different error types with helpful messages
    if (error.name === 'AbortError' || error.name === 'TimeoutError') {
      return res.status(504).json({
        success: false,
        error: 'Gateway Timeout',
        message: `ESP32 at ${ESP32_IP} did not respond within 15 seconds.`,
        possibleCauses: [
          'ESP32 is offline or not accessible from the internet',
          'ESP32_IP environment variable is incorrect',
          'Network firewall is blocking the connection',
          'ESP32 is on a local network (use ngrok or port forwarding)'
        ],
        solutions: [
          'Verify ESP32 is online and accessible',
          'Check ESP32_IP environment variable in Vercel dashboard',
          'Use ngrok: ngrok http 192.168.0.111:80',
          'Set up port forwarding on your router'
        ]
      });
    }
    
    if (error.code === 'ECONNREFUSED' || error.code === 'ENOTFOUND') {
      return res.status(503).json({
        success: false,
        error: 'Service Unavailable',
        message: `Cannot connect to ESP32 at ${ESP32_IP}.`,
        possibleCauses: [
          'ESP32 is not accessible from the internet',
          'ESP32_IP is incorrect or not set',
          'ESP32 is on a local network (192.168.x.x)',
          'DNS resolution failed'
        ],
        solutions: [
          'Set ESP32_IP environment variable in Vercel',
          'Use ngrok to create a public tunnel',
          'Configure port forwarding on your router',
          'Verify ESP32 is online and reachable'
        ]
      });
    }
    
    return res.status(500).json({
      success: false,
      error: 'Internal Server Error',
      message: error.message || 'Failed to proxy request to ESP32',
      details: process.env.NODE_ENV === 'development' ? error.stack : undefined
    });
  }
}

