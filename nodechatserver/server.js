require('dotenv').config({ path: './config.env' });
const express = require('express');
const http = require('http');
const socketIo = require('socket.io');
const mongoose = require('mongoose');
const cors = require('cors');
const rateLimit = require('express-rate-limit');
const helmet = require('helmet');
const jwt = require('jsonwebtoken');

const User = require('./models/User');
const Message = require('./models/Message');

const app = express();
const server = http.createServer(app);
const io = socketIo(server, {
  cors: {
    origin: "*",
    methods: ["GET", "POST"]
  }
});

// Middleware
app.use(helmet());
app.use(cors());
app.use(express.json());

// Rate limiting
const limiter = rateLimit({
  windowMs: 15 * 60 * 1000, // 15 minutes
  max: 100 // limit each IP to 100 requests per windowMs
});
app.use('/api/', limiter);

// Connect to MongoDB
mongoose.connect(process.env.MONGODB_URI, {
  useNewUrlParser: true,
  useUnifiedTopology: true
}).then(() => {
  console.log('Connected to MongoDB');
}).catch(err => {
  console.error('MongoDB connection error:', err);
});

// Online users tracking
const onlineUsers = new Map();

// Auth middleware for socket
const authenticateSocket = async (socket, next) => {
  try {
    const token = socket.handshake.auth.token;
    if (!token) {
      return next(new Error('No token provided'));
    }

    const decoded = jwt.verify(token, process.env.JWT_SECRET);
    const user = await User.findById(decoded.userId);
    
    if (!user) {
      return next(new Error('User not found'));
    }

    socket.userId = user._id;
    socket.username = user.username;
    next();
  } catch (error) {
    next(new Error('Authentication failed'));
  }
};

// API Routes
app.post('/api/register', async (req, res) => {
  try {
    const { username, password, discordName } = req.body;

    // Validation
    if (!username || !password) {
      return res.status(400).json({ 
        success: false, 
        message: 'Username and password are required' 
      });
    }

    if (username.length < 3 || username.length > 20) {
      return res.status(400).json({ 
        success: false, 
        message: 'Username must be between 3 and 20 characters' 
      });
    }

    if (password.length < 6) {
      return res.status(400).json({ 
        success: false, 
        message: 'Password must be at least 6 characters' 
      });
    }

    // Check if user already exists
    const existingUser = await User.findOne({ username });
    if (existingUser) {
      return res.status(400).json({ 
        success: false, 
        message: 'Username already exists' 
      });
    }

    // Create new user
    const user = new User({
      username,
      password,
      discordName: discordName || ''
    });

    await user.save();

    res.status(201).json({ 
      success: true, 
      message: 'User registered successfully' 
    });
  } catch (error) {
    console.error('Registration error:', error);
    res.status(500).json({ 
      success: false, 
      message: 'Internal server error' 
    });
  }
});

app.post('/api/login', async (req, res) => {
  try {
    const { username, password } = req.body;

    if (!username || !password) {
      return res.status(400).json({ 
        success: false, 
        message: 'Username and password are required' 
      });
    }

    // Find user
    const user = await User.findOne({ username });
    if (!user) {
      return res.status(401).json({ 
        success: false, 
        message: 'Invalid credentials' 
      });
    }

    // Check password
    const isPasswordValid = await user.comparePassword(password);
    if (!isPasswordValid) {
      return res.status(401).json({ 
        success: false, 
        message: 'Invalid credentials' 
      });
    }

    // Generate JWT token
    const token = jwt.sign(
      { userId: user._id, username: user.username },
      process.env.JWT_SECRET,
      { expiresIn: '24h' }
    );

    // Update user online status
    user.isOnline = true;
    await user.updateLastSeen();

    res.json({ 
      success: true, 
      token,
      user: {
        id: user._id,
        username: user.username,
        discordName: user.discordName
      }
    });
  } catch (error) {
    console.error('Login error:', error);
    res.status(500).json({ 
      success: false, 
      message: 'Internal server error' 
    });
  }
});

// Get recent messages
app.get('/api/messages', async (req, res) => {
  try {
    const messages = await Message.find({ type: 'general', isVisible: true })
      .sort({ timestamp: -1 })
      .limit(50)
      .select('username content timestamp');
    
    res.json({ success: true, messages: messages.reverse() });
  } catch (error) {
    console.error('Get messages error:', error);
    res.status(500).json({ success: false, message: 'Internal server error' });
  }
});

// Get online users
app.get('/api/users/online', async (req, res) => {
  try {
    const users = Array.from(onlineUsers.values());
    res.json({ success: true, users, count: users.length });
  } catch (error) {
    console.error('Get online users error:', error);
    res.status(500).json({ success: false, message: 'Internal server error' });
  }
});

// Send chat message via HTTP
app.post('/api/messages/send', async (req, res) => {
  try {
    const { content, token } = req.body;

    if (!token || !content) {
      return res.status(400).json({ success: false, message: 'Token and content required' });
    }

    // Verify token
    const decoded = jwt.verify(token, process.env.JWT_SECRET);
    const user = await User.findById(decoded.userId);
    
    if (!user) {
      return res.status(401).json({ success: false, message: 'Invalid token' });
    }

    if (content.trim().length === 0) {
      return res.status(400).json({ success: false, message: 'Message cannot be empty' });
    }

    if (content.length > 500) {
      return res.status(400).json({ success: false, message: 'Message too long' });
    }

    // Save message to database
    const message = new Message({
      username: user.username,
      content: content.trim(),
      type: 'general'
    });

    await message.save();

    // Broadcast to Socket.IO clients
    io.emit('chatMessage', {
      id: message._id,
      username: user.username,
      content: content.trim(),
      timestamp: message.timestamp
    });

    res.json({ success: true, message: 'Message sent' });
  } catch (error) {
    console.error('Send message error:', error);
    res.status(500).json({ success: false, message: 'Internal server error' });
  }
});

// Poll for new messages (for HTTP clients)
app.get('/api/messages/poll', async (req, res) => {
  try {
    const { since, token } = req.query;

    if (!token) {
      return res.status(400).json({ success: false, message: 'Token required' });
    }

    // Verify token
    const decoded = jwt.verify(token, process.env.JWT_SECRET);
    const user = await User.findById(decoded.userId);
    
    if (!user) {
      return res.status(401).json({ success: false, message: 'Invalid token' });
    }

    // Add/update user in online users (for HTTP polling clients)
    const userKey = user._id.toString();
    onlineUsers.set(userKey, {
      id: user._id,
      username: user.username,
      socketId: 'http-client-' + userKey,
      lastPoll: new Date()
    });

    // Clean up old HTTP clients (haven't polled in 30 seconds)
    const thirtySecondsAgo = new Date(Date.now() - 30000);
    for (const [key, userData] of onlineUsers.entries()) {
      if (userData.socketId && userData.socketId.startsWith('http-client-') && 
          userData.lastPoll && userData.lastPoll < thirtySecondsAgo) {
        onlineUsers.delete(key);
      }
    }

    // Get messages since timestamp
    const sinceDate = since ? new Date(since) : new Date(Date.now() - 60000); // Last minute if no timestamp
    
    const messages = await Message.find({ 
      type: 'general', 
      isVisible: true,
      timestamp: { $gt: sinceDate }
    })
    .sort({ timestamp: 1 })
    .limit(50)
    .select('username content timestamp');
    
    res.json({ 
      success: true, 
      messages,
      timestamp: new Date().toISOString(),
      onlineUsers: Array.from(onlineUsers.values()),
      onlineCount: onlineUsers.size
    });
  } catch (error) {
    console.error('Poll messages error:', error);
    res.status(500).json({ success: false, message: 'Internal server error' });
  }
});

// Socket.IO events
io.use(authenticateSocket);

io.on('connection', async (socket) => {
  console.log(`User ${socket.username} connected`);

  // Add user to online users
  onlineUsers.set(socket.userId.toString(), {
    id: socket.userId,
    username: socket.username,
    socketId: socket.id
  });

  // Update user online status in database
  await User.findByIdAndUpdate(socket.userId, { isOnline: true });

  // Broadcast user joined
  socket.broadcast.emit('userJoined', {
    username: socket.username,
    onlineCount: onlineUsers.size
  });

  // Send current online users to the new user
  socket.emit('onlineUsers', {
    users: Array.from(onlineUsers.values()),
    count: onlineUsers.size
  });

  // Handle chat messages
  socket.on('chatMessage', async (data) => {
    try {
      const { content } = data;

      if (!content || content.trim().length === 0) {
        return;
      }

      if (content.length > 500) {
        socket.emit('error', { message: 'Message too long' });
        return;
      }

      // Save message to database
      const message = new Message({
        username: socket.username,
        content: content.trim(),
        type: 'general'
      });

      await message.save();

      // Broadcast message to all users
      io.emit('chatMessage', {
        id: message._id,
        username: socket.username,
        content: content.trim(),
        timestamp: message.timestamp
      });
    } catch (error) {
      console.error('Chat message error:', error);
      socket.emit('error', { message: 'Failed to send message' });
    }
  });

  // Handle private messages
  socket.on('privateMessage', async (data) => {
    try {
      const { recipient, content } = data;

      if (!recipient || !content || content.trim().length === 0) {
        return;
      }

      if (content.length > 500) {
        socket.emit('error', { message: 'Message too long' });
        return;
      }

      // Find recipient
      const recipientUser = Array.from(onlineUsers.values())
        .find(user => user.username === recipient);

      if (!recipientUser) {
        socket.emit('error', { message: 'User not online' });
        return;
      }

      const privateMessage = {
        from: socket.username,
        to: recipient,
        content: content.trim(),
        timestamp: new Date()
      };

      // Send to recipient
      const recipientSocket = io.sockets.sockets.get(recipientUser.socketId);
      if (recipientSocket) {
        recipientSocket.emit('privateMessage', privateMessage);
      }

      // Send confirmation to sender
      socket.emit('privateMessage', privateMessage);
    } catch (error) {
      console.error('Private message error:', error);
      socket.emit('error', { message: 'Failed to send private message' });
    }
  });

  // Handle typing indicator
  socket.on('typing', (data) => {
    socket.broadcast.emit('userTyping', {
      username: socket.username,
      isTyping: data.isTyping
    });
  });

  // Handle disconnect
  socket.on('disconnect', async () => {
    console.log(`User ${socket.username} disconnected`);

    // Remove user from online users
    onlineUsers.delete(socket.userId.toString());

    // Update user online status in database
    await User.findByIdAndUpdate(socket.userId, { 
      isOnline: false,
      lastSeen: new Date()
    });

    // Broadcast user left
    socket.broadcast.emit('userLeft', {
      username: socket.username,
      onlineCount: onlineUsers.size
    });
  });
});

// Health check endpoint
app.get('/health', (req, res) => {
  res.json({ 
    status: 'ok', 
    timestamp: new Date().toISOString(),
    onlineUsers: onlineUsers.size
  });
});

const PORT = process.env.PORT || 3678;
server.listen(PORT, () => {
  console.log(`RME Chat Server running on port ${PORT}`);
}); 