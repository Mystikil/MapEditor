# RME Chat Server

A Node.js chat server for Remere's Map Editor with user authentication, real-time messaging, and MongoDB integration.

## Features

- User registration and authentication
- Real-time chat messaging
- Private messaging between users
- Online user tracking
- Discord name integration (optional)
- MongoDB database for persistent storage
- JWT-based authentication
- Rate limiting for security

## Prerequisites

- Node.js (v16 or higher)
- MongoDB (local installation or MongoDB Atlas)
- npm or yarn

## Setup

1. **Install Dependencies**
   ```bash
   npm install
   ```

2. **Configure Environment**
   - Copy `config.env` and update the values:
     - `MONGODB_URI`: Your MongoDB connection string
     - `JWT_SECRET`: A secure secret key for JWT tokens
     - `PORT`: Server port (default: 3678)

3. **Start MongoDB**
   Make sure MongoDB is running on your system or configure a remote MongoDB instance.

4. **Run the Server**
   ```bash
   # Development mode
   npm run dev
   
   # Production mode
   npm start
   ```

## API Endpoints

### Authentication
- `POST /api/register` - Register a new user
  ```json
  {
    "username": "string",
    "password": "string",
    "discordName": "string (optional)"
  }
  ```

- `POST /api/login` - Login user
  ```json
  {
    "username": "string",
    "password": "string"
  }
  ```

### Chat
- `GET /api/messages` - Get recent chat messages
- `GET /api/users/online` - Get currently online users
- `GET /health` - Health check endpoint

## Socket.IO Events

### Client to Server
- `chatMessage` - Send a message to general chat
- `privateMessage` - Send a private message to a specific user
- `typing` - Indicate typing status

### Server to Client
- `chatMessage` - Receive a general chat message
- `privateMessage` - Receive a private message
- `userJoined` - User joined notification
- `userLeft` - User left notification
- `onlineUsers` - List of online users
- `userTyping` - Typing indicator
- `error` - Error message

## Database Schema

### User Model
- `username` (string, unique, required)
- `password` (string, hashed, required)
- `discordName` (string, optional)
- `isOnline` (boolean)
- `lastSeen` (date)
- `joinDate` (date)

### Message Model
- `username` (string, required)
- `content` (string, required)
- `type` (enum: general/private/system)
- `recipient` (string, for private messages)
- `timestamp` (date)
- `isVisible` (boolean)

## Security Features

- Password hashing with bcrypt
- JWT token authentication
- Rate limiting
- CORS protection
- Helmet security headers
- Input validation

## Development

```bash
# Install dev dependencies
npm install --save-dev nodemon

# Run in development mode with auto-restart
npm run dev
```

## Production Deployment

1. Set `NODE_ENV=production` in your environment
2. Use a secure `JWT_SECRET`
3. Configure MongoDB with proper authentication
4. Use process managers like PM2 for production deployment

```bash
# Install PM2
npm install -g pm2

# Start with PM2
pm2 start server.js --name "rme-chat-server"
``` 