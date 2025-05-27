const mongoose = require('mongoose');

const messageSchema = new mongoose.Schema({
  username: {
    type: String,
    required: true,
    trim: true
  },
  content: {
    type: String,
    required: true,
    maxlength: 500
  },
  type: {
    type: String,
    enum: ['general', 'private', 'system'],
    default: 'general'
  },
  recipient: {
    type: String,
    trim: true,
    default: null // For private messages
  },
  timestamp: {
    type: Date,
    default: Date.now
  },
  isVisible: {
    type: Boolean,
    default: true
  }
}, {
  timestamps: true
});

// Index for efficient querying
messageSchema.index({ type: 1, timestamp: -1 });
messageSchema.index({ recipient: 1, timestamp: -1 });

module.exports = mongoose.model('Message', messageSchema); 